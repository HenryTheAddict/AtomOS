/*
 * AtomOS - Global Descriptor Table (GDT)
 * x86 memory segmentation setup
 */

#ifndef _ATOMOS_GDT_H
#define _ATOMOS_GDT_H

#include "../../include/types.h"

/* GDT segment selectors */
#define GDT_NULL        0x00
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20
#define GDT_TSS         0x28

/* GDT entry structure */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} PACKED gdt_entry_t;

/* GDT pointer structure */
typedef struct {
    uint16_t limit;
    uint32_t base;
} PACKED gdt_ptr_t;

/* Task State Segment */
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;      /* Kernel stack pointer */
    uint32_t ss0;       /* Kernel stack segment */
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} PACKED tss_entry_t;

/* GDT access byte flags */
#define GDT_ACCESS_PRESENT      0x80
#define GDT_ACCESS_RING0        0x00
#define GDT_ACCESS_RING1        0x20
#define GDT_ACCESS_RING2        0x40
#define GDT_ACCESS_RING3        0x60
#define GDT_ACCESS_SYSTEM       0x00
#define GDT_ACCESS_CODEDATA     0x10
#define GDT_ACCESS_EXECUTABLE   0x08
#define GDT_ACCESS_DC           0x04
#define GDT_ACCESS_RW           0x02
#define GDT_ACCESS_ACCESSED     0x01

/* GDT granularity flags */
#define GDT_GRAN_4K             0x80
#define GDT_GRAN_32BIT          0x40

/* Function declarations */
void gdt_init(void);
void gdt_set_entry(int num, uint32_t base, uint32_t limit, 
                   uint8_t access, uint8_t gran);
void tss_set_kernel_stack(uint32_t stack);

/* Assembly function */
extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush(void);

#endif /* _ATOMOS_GDT_H */
