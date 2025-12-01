/*
 * AtomOS - Virtual Memory Manager Implementation
 * x86 paging implementation
 */

#include "vmm.h"
#include "pmm.h"
#include "../include/kernel.h"
#include "../arch/x86/idt.h"

/* Current and kernel page directories */
static page_directory_t *current_directory = NULL;
static page_directory_t *kernel_directory = NULL;

/*
 * Flush TLB for a single page
 */
void vmm_flush_tlb_page(uint32_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/*
 * Flush entire TLB
 */
void vmm_flush_tlb(void) {
    __asm__ volatile(
        "mov %%cr3, %%eax\n"
        "mov %%eax, %%cr3"
        ::: "eax", "memory"
    );
}

/*
 * Switch to a new address space
 */
void vmm_switch_address_space(page_directory_t *dir) {
    current_directory = dir;
    __asm__ volatile("mov %0, %%cr3" : : "r"(dir) : "memory");
}

/*
 * Get current page directory
 */
page_directory_t *vmm_get_current_directory(void) {
    return current_directory;
}

/*
 * Get kernel page directory
 */
page_directory_t *vmm_get_kernel_directory(void) {
    return kernel_directory;
}

/*
 * Get or create page table for a virtual address
 */
static page_table_t *vmm_get_page_table(page_directory_t *dir, uint32_t virt, bool create) {
    uint32_t dir_idx = VMM_DIR_INDEX(virt);
    
    if (dir->entries[dir_idx] & PTE_PRESENT) {
        return (page_table_t *)(dir->entries[dir_idx] & PAGE_FRAME_MASK);
    }
    
    if (!create) {
        return NULL;
    }
    
    /* Allocate new page table */
    page_table_t *table = (page_table_t *)pmm_alloc_page();
    if (!table) {
        return NULL;
    }
    
    memset(table, 0, sizeof(page_table_t));
    
    /* Add to page directory */
    dir->entries[dir_idx] = ((uint32_t)table) | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    
    return table;
}

/*
 * Map a virtual page to a physical page
 */
void vmm_map_page(page_directory_t *dir, uint32_t virt, uint32_t phys, uint32_t flags) {
    page_table_t *table = vmm_get_page_table(dir, virt, true);
    if (!table) {
        kerror("VMM: Failed to get page table for 0x%x\n", virt);
        return;
    }
    
    uint32_t table_idx = VMM_TABLE_INDEX(virt);
    table->entries[table_idx] = (phys & PAGE_FRAME_MASK) | (flags & PAGE_FLAGS_MASK) | PTE_PRESENT;
    
    /* Flush TLB for this page if it's in current address space */
    if (dir == current_directory) {
        vmm_flush_tlb_page(virt);
    }
}

/*
 * Unmap a virtual page
 */
void vmm_unmap_page(page_directory_t *dir, uint32_t virt) {
    page_table_t *table = vmm_get_page_table(dir, virt, false);
    if (!table) {
        return;
    }
    
    uint32_t table_idx = VMM_TABLE_INDEX(virt);
    table->entries[table_idx] = 0;
    
    if (dir == current_directory) {
        vmm_flush_tlb_page(virt);
    }
}

/*
 * Get physical address for a virtual address
 */
uint32_t vmm_get_physical(page_directory_t *dir, uint32_t virt) {
    page_table_t *table = vmm_get_page_table(dir, virt, false);
    if (!table) {
        return 0;
    }
    
    uint32_t table_idx = VMM_TABLE_INDEX(virt);
    if (!(table->entries[table_idx] & PTE_PRESENT)) {
        return 0;
    }
    
    return (table->entries[table_idx] & PAGE_FRAME_MASK) | VMM_PAGE_OFFSET(virt);
}

/*
 * Check if a virtual address is mapped
 */
bool vmm_is_mapped(page_directory_t *dir, uint32_t virt) {
    page_table_t *table = vmm_get_page_table(dir, virt, false);
    if (!table) {
        return false;
    }
    
    uint32_t table_idx = VMM_TABLE_INDEX(virt);
    return (table->entries[table_idx] & PTE_PRESENT) != 0;
}

/*
 * Map a range of pages
 */
void vmm_map_range(page_directory_t *dir, uint32_t virt_start, 
                   uint32_t phys_start, uint32_t size, uint32_t flags) {
    size = ALIGN_UP(size, PAGE_SIZE);
    
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        vmm_map_page(dir, virt_start + offset, phys_start + offset, flags);
    }
}

/*
 * Unmap a range of pages
 */
void vmm_unmap_range(page_directory_t *dir, uint32_t virt_start, uint32_t size) {
    size = ALIGN_UP(size, PAGE_SIZE);
    
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        vmm_unmap_page(dir, virt_start + offset);
    }
}

/*
 * Allocate a virtual page
 */
void *vmm_alloc_page(uint32_t flags) {
    void *phys = pmm_alloc_page();
    if (!phys) {
        return NULL;
    }
    
    /* For kernel allocations, virtual = physical (identity mapped) */
    vmm_map_page(current_directory, (uint32_t)phys, (uint32_t)phys, flags | PTE_WRITABLE);
    
    return phys;
}

/*
 * Allocate multiple virtual pages
 */
void *vmm_alloc_pages(uint32_t count, uint32_t flags) {
    void *phys = pmm_alloc_pages(count);
    if (!phys) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t addr = (uint32_t)phys + i * PAGE_SIZE;
        vmm_map_page(current_directory, addr, addr, flags | PTE_WRITABLE);
    }
    
    return phys;
}

/*
 * Free a virtual page
 */
void vmm_free_page(void *addr) {
    vmm_unmap_page(current_directory, (uint32_t)addr);
    pmm_free_page(addr);
}

/*
 * Free multiple virtual pages
 */
void vmm_free_pages(void *addr, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vmm_free_page((void *)((uint32_t)addr + i * PAGE_SIZE));
    }
}

/*
 * Create a new address space
 */
page_directory_t *vmm_create_address_space(void) {
    page_directory_t *dir = (page_directory_t *)pmm_alloc_page();
    if (!dir) {
        return NULL;
    }
    
    memset(dir, 0, sizeof(page_directory_t));
    
    /* Copy kernel page tables to new address space */
    if (kernel_directory) {
        for (int i = 768; i < 1024; i++) {  /* Upper 1GB is kernel space */
            dir->entries[i] = kernel_directory->entries[i];
        }
    }
    
    return dir;
}

/*
 * Destroy an address space
 */
void vmm_destroy_address_space(page_directory_t *dir) {
    if (!dir || dir == kernel_directory) {
        return;
    }
    
    /* Free user-space page tables */
    for (int i = 0; i < 768; i++) {
        if (dir->entries[i] & PTE_PRESENT) {
            page_table_t *table = (page_table_t *)(dir->entries[i] & PAGE_FRAME_MASK);
            
            /* Free mapped pages */
            for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                if (table->entries[j] & PTE_PRESENT) {
                    pmm_free_page((void *)(table->entries[j] & PAGE_FRAME_MASK));
                }
            }
            
            pmm_free_page(table);
        }
    }
    
    pmm_free_page(dir);
}

/*
 * Clone an address space (for fork)
 */
page_directory_t *vmm_clone_address_space(page_directory_t *src) {
    page_directory_t *dst = vmm_create_address_space();
    if (!dst) {
        return NULL;
    }
    
    /* Clone user-space page tables */
    for (int i = 0; i < 768; i++) {
        if (src->entries[i] & PTE_PRESENT) {
            page_table_t *src_table = (page_table_t *)(src->entries[i] & PAGE_FRAME_MASK);
            page_table_t *dst_table = (page_table_t *)pmm_alloc_page();
            
            if (!dst_table) {
                vmm_destroy_address_space(dst);
                return NULL;
            }
            
            /* Copy page table entries and pages */
            for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                if (src_table->entries[j] & PTE_PRESENT) {
                    void *new_page = pmm_alloc_page();
                    if (new_page) {
                        void *src_page = (void *)(src_table->entries[j] & PAGE_FRAME_MASK);
                        memcpy(new_page, src_page, PAGE_SIZE);
                        dst_table->entries[j] = ((uint32_t)new_page) | 
                            (src_table->entries[j] & PAGE_FLAGS_MASK);
                    }
                } else {
                    dst_table->entries[j] = 0;
                }
            }
            
            dst->entries[i] = ((uint32_t)dst_table) | (src->entries[i] & PAGE_FLAGS_MASK);
        }
    }
    
    return dst;
}

/*
 * Page fault handler
 */
static void page_fault_handler(registers_t *regs) {
    /* Get faulting address from CR2 */
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
    
    /* Decode error code */
    bool present = regs->err_code & 0x1;
    bool write = regs->err_code & 0x2;
    bool user = regs->err_code & 0x4;
    bool reserved = regs->err_code & 0x8;
    bool instruction = regs->err_code & 0x10;
    
    kerror("Page Fault at 0x%x\n", fault_addr);
    kerror("  Present: %d, Write: %d, User: %d, Reserved: %d, Instruction: %d\n",
           present, write, user, reserved, instruction);
    kerror("  EIP: 0x%x\n", regs->eip);
    
    /* For now, panic on page faults */
    panic("Page Fault");
}

/*
 * Initialize the virtual memory manager
 */
void vmm_init(void) {
    /* Create kernel page directory */
    kernel_directory = (page_directory_t *)pmm_alloc_page();
    if (!kernel_directory) {
        panic("Failed to allocate kernel page directory");
    }
    
    memset(kernel_directory, 0, sizeof(page_directory_t));
    
    /* Identity map first 16MB for kernel */
    for (uint32_t addr = 0; addr < MB(16); addr += PAGE_SIZE) {
        vmm_map_page(kernel_directory, addr, addr, PTE_WRITABLE);
    }
    
    /* Register page fault handler */
    register_interrupt_handler(14, page_fault_handler);
    
    /* Switch to kernel page directory */
    vmm_switch_address_space(kernel_directory);
    
    /* Enable paging */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  /* Set PG bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    kprintf("VMM initialized, paging enabled\n");
}
