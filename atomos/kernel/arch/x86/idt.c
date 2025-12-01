/*
 * AtomOS - IDT Implementation
 * Interrupt Descriptor Table and interrupt handling
 */

#include "idt.h"
#include "gdt.h"
#include "../../include/kernel.h"

/* IDT entries and pointer */
static idt_entry_t idt_entries[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

/* Interrupt handlers */
static isr_handler_t interrupt_handlers[IDT_ENTRIES];

/* Exception names for debugging */
const char *exception_names[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

/*
 * Set an IDT entry
 */
void idt_set_entry(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector = sel;
    idt_entries[num].zero = 0;
    idt_entries[num].flags = flags | 0x80; /* Set present bit */
}

/*
 * Initialize the PIC (Programmable Interrupt Controller)
 */
void pic_init(void) {
    /* ICW1: Start initialization, cascade mode */
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    
    /* ICW2: Remap IRQs to 32-47 */
    outb(PIC1_DATA, 0x20);  /* IRQ 0-7 -> INT 32-39 */
    outb(PIC2_DATA, 0x28);  /* IRQ 8-15 -> INT 40-47 */
    
    /* ICW3: Cascade configuration */
    outb(PIC1_DATA, 0x04);  /* Slave at IRQ2 */
    outb(PIC2_DATA, 0x02);  /* Cascade identity */
    
    /* ICW4: 8086 mode */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    /* Mask all interrupts initially */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    
    kprintf("PIC remapped: IRQ 0-15 -> INT 32-47\n");
}

/*
 * Send End Of Interrupt signal
 */
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

/*
 * Enable a specific IRQ
 */
void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

/*
 * Disable a specific IRQ
 */
void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

/*
 * Register an interrupt handler
 */
void register_interrupt_handler(uint8_t num, isr_handler_t handler) {
    interrupt_handlers[num] = handler;
}

/*
 * Common interrupt handler (called from assembly stubs)
 */
void isr_handler(registers_t *regs) {
    /* Call registered handler if exists */
    if (interrupt_handlers[regs->int_no] != NULL) {
        interrupt_handlers[regs->int_no](regs);
    } else if (regs->int_no < 32) {
        /* Unhandled exception - panic */
        kerror("Unhandled exception: %s (int %d)\n", 
               exception_names[regs->int_no], regs->int_no);
        kerror("Error code: 0x%x\n", regs->err_code);
        kerror("EIP: 0x%x, CS: 0x%x\n", regs->eip, regs->cs);
        kerror("EFLAGS: 0x%x\n", regs->eflags);
        panic("Unhandled exception");
    }
}

/*
 * Common IRQ handler (called from assembly stubs)
 */
void irq_handler(registers_t *regs) {
    /* Send EOI first for nested interrupts */
    pic_send_eoi(regs->int_no - 32);
    
    /* Call registered handler if exists */
    if (interrupt_handlers[regs->int_no] != NULL) {
        interrupt_handlers[regs->int_no](regs);
    }
}

/*
 * Initialize the IDT
 */
void idt_init(void) {
    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base = (uint32_t)&idt_entries;
    
    /* Clear IDT and handlers */
    memset(&idt_entries, 0, sizeof(idt_entries));
    memset(&interrupt_handlers, 0, sizeof(interrupt_handlers));
    
    /* Initialize PIC */
    pic_init();
    
    /* Set up exception handlers (ISR 0-31) */
    idt_set_entry(0,  (uint32_t)isr0,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(1,  (uint32_t)isr1,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(2,  (uint32_t)isr2,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(3,  (uint32_t)isr3,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(4,  (uint32_t)isr4,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(5,  (uint32_t)isr5,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(6,  (uint32_t)isr6,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(7,  (uint32_t)isr7,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(8,  (uint32_t)isr8,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(9,  (uint32_t)isr9,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(10, (uint32_t)isr10, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(11, (uint32_t)isr11, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(12, (uint32_t)isr12, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(13, (uint32_t)isr13, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(14, (uint32_t)isr14, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(15, (uint32_t)isr15, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(16, (uint32_t)isr16, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(17, (uint32_t)isr17, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(18, (uint32_t)isr18, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(19, (uint32_t)isr19, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(20, (uint32_t)isr20, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(21, (uint32_t)isr21, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(22, (uint32_t)isr22, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(23, (uint32_t)isr23, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(24, (uint32_t)isr24, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(25, (uint32_t)isr25, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(26, (uint32_t)isr26, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(27, (uint32_t)isr27, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(28, (uint32_t)isr28, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(29, (uint32_t)isr29, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(30, (uint32_t)isr30, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(31, (uint32_t)isr31, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    
    /* Set up IRQ handlers (IRQ 0-15 -> INT 32-47) */
    idt_set_entry(32, (uint32_t)irq0,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(33, (uint32_t)irq1,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(34, (uint32_t)irq2,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(35, (uint32_t)irq3,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(36, (uint32_t)irq4,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(37, (uint32_t)irq5,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(38, (uint32_t)irq6,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(39, (uint32_t)irq7,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(40, (uint32_t)irq8,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(41, (uint32_t)irq9,  GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(42, (uint32_t)irq10, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(43, (uint32_t)irq11, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(44, (uint32_t)irq12, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(45, (uint32_t)irq13, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(46, (uint32_t)irq14, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    idt_set_entry(47, (uint32_t)irq15, GDT_KERNEL_CODE, IDT_INT_GATE_32);
    
    /* System call interrupt (Ring 3 accessible) */
    idt_set_entry(128, (uint32_t)isr128, GDT_KERNEL_CODE, 
                  IDT_INT_GATE_32 | 0x60);
    
    /* Load IDT */
    idt_flush((uint32_t)&idt_ptr);
    
    /* Enable timer and keyboard interrupts */
    pic_clear_mask(0);  /* Timer */
    pic_clear_mask(1);  /* Keyboard */
    
    kprintf("IDT initialized with %d entries\n", IDT_ENTRIES);
}
