/*
 * AtomOS - GDT Implementation
 * Global Descriptor Table setup for x86
 */

#include "gdt.h"
#include "../../include/kernel.h"

/* GDT entries (6 entries: null, kernel code/data, user code/data, TSS) */
static gdt_entry_t gdt_entries[6];
static gdt_ptr_t gdt_ptr;
static tss_entry_t tss;

/*
 * Set a GDT entry
 */
void gdt_set_entry(int num, uint32_t base, uint32_t limit, 
                   uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low = base & 0xFFFF;
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;
    
    gdt_entries[num].limit_low = limit & 0xFFFF;
    gdt_entries[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    
    gdt_entries[num].access = access;
}

/*
 * Set TSS entry in GDT
 */
static void tss_write(int num, uint32_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss_entry_t);
    
    /* Clear TSS */
    memset(&tss, 0, sizeof(tss));
    
    /* Set kernel stack */
    tss.ss0 = ss0;
    tss.esp0 = esp0;
    
    /* Set I/O map base to sizeof(tss) to indicate no I/O bitmap */
    tss.iomap_base = sizeof(tss_entry_t);
    
    /* Set in GDT */
    gdt_set_entry(num, base, limit, 0xE9, 0x00);
}

/*
 * Set kernel stack in TSS (for ring transitions)
 */
void tss_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}

/*
 * Initialize the GDT
 */
void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;
    
    /* Null segment */
    gdt_set_entry(0, 0, 0, 0, 0);
    
    /* Kernel code segment: base=0, limit=4GB, ring 0 */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODEDATA | 
        GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_32BIT);
    
    /* Kernel data segment: base=0, limit=4GB, ring 0 */
    gdt_set_entry(2, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODEDATA | 
        GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_32BIT);
    
    /* User code segment: base=0, limit=4GB, ring 3 */
    gdt_set_entry(3, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODEDATA | 
        GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_32BIT);
    
    /* User data segment: base=0, limit=4GB, ring 3 */
    gdt_set_entry(4, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODEDATA | 
        GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_32BIT);
    
    /* TSS segment */
    tss_write(5, GDT_KERNEL_DATA, 0x90000);
    
    /* Load GDT and TSS */
    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
    
    kprintf("GDT initialized with %d entries\n", 6);
}
