/*
 * AtomOS - Interrupt Descriptor Table (IDT)
 * x86 interrupt handling setup
 */

#ifndef _ATOMOS_IDT_H
#define _ATOMOS_IDT_H

#include "../../include/types.h"

/* Number of IDT entries */
#define IDT_ENTRIES 256

/* IDT entry types */
#define IDT_TASK_GATE_32    0x05
#define IDT_INT_GATE_16     0x06
#define IDT_TRAP_GATE_16    0x07
#define IDT_INT_GATE_32     0x0E
#define IDT_TRAP_GATE_32    0x0F

/* IDT entry structure */
typedef struct {
    uint16_t base_low;      /* Lower 16 bits of handler address */
    uint16_t selector;      /* Kernel segment selector */
    uint8_t  zero;          /* Always zero */
    uint8_t  flags;         /* Type and attributes */
    uint16_t base_high;     /* Upper 16 bits of handler address */
} PACKED idt_entry_t;

/* IDT pointer structure */
typedef struct {
    uint16_t limit;
    uint32_t base;
} PACKED idt_ptr_t;

/* CPU registers pushed during interrupt */
typedef struct {
    /* Pushed by pusha */
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    
    /* Pushed by interrupt handler stub */
    uint32_t int_no, err_code;
    
    /* Pushed by CPU */
    uint32_t eip, cs, eflags, esp, ss;
} PACKED registers_t;

/* Interrupt handler function type */
typedef void (*isr_handler_t)(registers_t *regs);

/* Exception names */
extern const char *exception_names[];

/* Function declarations */
void idt_init(void);
void idt_set_entry(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void register_interrupt_handler(uint8_t num, isr_handler_t handler);

/* PIC (Programmable Interrupt Controller) */
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

#define PIC_EOI     0x20    /* End of interrupt */

/* Hardware IRQ numbers (after remapping) */
#define IRQ0        32      /* Timer */
#define IRQ1        33      /* Keyboard */
#define IRQ2        34      /* Cascade */
#define IRQ3        35      /* COM2 */
#define IRQ4        36      /* COM1 */
#define IRQ5        37      /* LPT2 */
#define IRQ6        38      /* Floppy */
#define IRQ7        39      /* LPT1 */
#define IRQ8        40      /* CMOS RTC */
#define IRQ9        41      /* Free */
#define IRQ10       42      /* Free */
#define IRQ11       43      /* Free */
#define IRQ12       44      /* PS/2 Mouse */
#define IRQ13       45      /* FPU */
#define IRQ14       46      /* Primary ATA */
#define IRQ15       47      /* Secondary ATA */

/* Syscall interrupt */
#define SYSCALL_INT 0x80

void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

/* Assembly ISR stubs (declared in isr_asm.asm) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

extern void isr128(void);   /* Syscall */

extern void idt_flush(uint32_t idt_ptr);

#endif /* _ATOMOS_IDT_H */
