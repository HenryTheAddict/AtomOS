/*
 * AtomOS - Virtual Memory Manager
 * Paging and virtual address space management
 */

#ifndef _ATOMOS_VMM_H
#define _ATOMOS_VMM_H

#include "../include/types.h"

/* Page table entry flags */
#define PTE_PRESENT     0x001
#define PTE_WRITABLE    0x002
#define PTE_USER        0x004
#define PTE_WRITETHROUGH 0x008
#define PTE_NOCACHE     0x010
#define PTE_ACCESSED    0x020
#define PTE_DIRTY       0x040
#define PTE_HUGE        0x080   /* 4MB page (PDE only) */
#define PTE_GLOBAL      0x100

/* Page directory/table sizes */
#define PAGE_DIR_ENTRIES    1024
#define PAGE_TABLE_ENTRIES  1024

/* Address masks */
#define PAGE_FRAME_MASK     0xFFFFF000
#define PAGE_FLAGS_MASK     0x00000FFF

/* Virtual address breakdown */
#define VMM_DIR_INDEX(addr)     (((addr) >> 22) & 0x3FF)
#define VMM_TABLE_INDEX(addr)   (((addr) >> 12) & 0x3FF)
#define VMM_PAGE_OFFSET(addr)   ((addr) & 0xFFF)

/* Kernel virtual address base */
#define KERNEL_VIRT_BASE    0xC0000000  /* 3GB mark */
#define KERNEL_HEAP_START   0xC0400000  /* Kernel heap starts at 3GB + 4MB */
#define KERNEL_HEAP_END     0xCFFFFFFF  /* Kernel heap max */

/* User space limits */
#define USER_SPACE_START    0x00400000  /* 4MB - below reserved */
#define USER_SPACE_END      0xBFFFFFFF  /* Below kernel */
#define USER_STACK_TOP      0xBFFFF000  /* User stack top */

/* Page directory entry */
typedef uint32_t pde_t;

/* Page table entry */
typedef uint32_t pte_t;

/* Page directory structure */
typedef struct {
    pde_t entries[PAGE_DIR_ENTRIES];
} page_directory_t;

/* Page table structure */
typedef struct {
    pte_t entries[PAGE_TABLE_ENTRIES];
} page_table_t;

/* Virtual memory region */
typedef struct vmm_region {
    uint32_t start;
    uint32_t end;
    uint32_t flags;
    struct vmm_region *next;
} vmm_region_t;

/* Address space structure */
typedef struct {
    page_directory_t *page_dir;
    uint32_t page_dir_phys;
    vmm_region_t *regions;
} address_space_t;

/* Function declarations */
void vmm_init(void);

/* Page directory operations */
page_directory_t *vmm_create_address_space(void);
void vmm_destroy_address_space(page_directory_t *dir);
void vmm_switch_address_space(page_directory_t *dir);
page_directory_t *vmm_get_current_directory(void);
page_directory_t *vmm_get_kernel_directory(void);

/* Page mapping */
void vmm_map_page(page_directory_t *dir, uint32_t virt, uint32_t phys, uint32_t flags);
void vmm_unmap_page(page_directory_t *dir, uint32_t virt);
uint32_t vmm_get_physical(page_directory_t *dir, uint32_t virt);
bool vmm_is_mapped(page_directory_t *dir, uint32_t virt);

/* Range mapping */
void vmm_map_range(page_directory_t *dir, uint32_t virt_start, 
                   uint32_t phys_start, uint32_t size, uint32_t flags);
void vmm_unmap_range(page_directory_t *dir, uint32_t virt_start, uint32_t size);

/* Memory allocation */
void *vmm_alloc_page(uint32_t flags);
void *vmm_alloc_pages(uint32_t count, uint32_t flags);
void vmm_free_page(void *addr);
void vmm_free_pages(void *addr, uint32_t count);

/* Page fault handler */
void vmm_page_fault_handler(uint32_t fault_addr, uint32_t error_code);

/* TLB operations */
void vmm_flush_tlb(void);
void vmm_flush_tlb_page(uint32_t virt);

/* Clone address space for fork */
page_directory_t *vmm_clone_address_space(page_directory_t *src);

#endif /* _ATOMOS_VMM_H */
