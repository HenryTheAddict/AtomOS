/*
 * AtomOS - Physical Memory Manager Implementation
 * Bitmap-based physical page allocator
 */

#include "pmm.h"
#include "../include/kernel.h"

/* Bitmap for tracking page allocation */
static uint32_t *pmm_bitmap;
static uint32_t pmm_bitmap_size;
static uint32_t pmm_total_pages;
static uint32_t pmm_used_pages;

/* Memory bounds */
static uint32_t pmm_memory_size;
static uint32_t pmm_first_free_page;

/*
 * Set a bit in the bitmap (mark page as used)
 */
static void pmm_set_bit(uint32_t page) {
    pmm_bitmap[page / 32] |= (1 << (page % 32));
}

/*
 * Clear a bit in the bitmap (mark page as free)
 */
static void pmm_clear_bit(uint32_t page) {
    pmm_bitmap[page / 32] &= ~(1 << (page % 32));
}

/*
 * Test if a bit is set
 */
static bool pmm_test_bit(uint32_t page) {
    return pmm_bitmap[page / 32] & (1 << (page % 32));
}

/*
 * Find first free page
 */
static int32_t pmm_find_free_page(void) {
    for (uint32_t i = pmm_first_free_page / 32; i < pmm_bitmap_size; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(pmm_bitmap[i] & (1 << j))) {
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

/*
 * Find contiguous free pages
 */
static int32_t pmm_find_free_pages(uint32_t count) {
    uint32_t found = 0;
    int32_t start = -1;
    
    for (uint32_t page = pmm_first_free_page; page < pmm_total_pages; page++) {
        if (!pmm_test_bit(page)) {
            if (found == 0) {
                start = page;
            }
            found++;
            if (found == count) {
                return start;
            }
        } else {
            found = 0;
            start = -1;
        }
    }
    
    return -1;
}

/*
 * Mark a region of memory as used
 */
void pmm_mark_region_used(uint32_t base, uint32_t size) {
    uint32_t start_page = ALIGN_DOWN(base, PAGE_SIZE) / PAGE_SIZE;
    uint32_t page_count = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    
    for (uint32_t i = 0; i < page_count; i++) {
        if (start_page + i < pmm_total_pages) {
            if (!pmm_test_bit(start_page + i)) {
                pmm_set_bit(start_page + i);
                pmm_used_pages++;
            }
        }
    }
}

/*
 * Mark a region of memory as free
 */
void pmm_mark_region_free(uint32_t base, uint32_t size) {
    uint32_t start_page = ALIGN_UP(base, PAGE_SIZE) / PAGE_SIZE;
    uint32_t page_count = size / PAGE_SIZE;
    
    for (uint32_t i = 0; i < page_count; i++) {
        if (start_page + i < pmm_total_pages && start_page + i > 0) {
            if (pmm_test_bit(start_page + i)) {
                pmm_clear_bit(start_page + i);
                pmm_used_pages--;
            }
        }
    }
}

/*
 * Initialize the physical memory manager
 */
void pmm_init(uint32_t memory_size, uint32_t *memory_map, uint32_t entries) {
    pmm_memory_size = memory_size;
    pmm_total_pages = memory_size / PAGE_SIZE;
    pmm_bitmap_size = pmm_total_pages / 32;
    if (pmm_total_pages % 32) pmm_bitmap_size++;
    
    /* Place bitmap after kernel (at 2MB mark for safety) */
    pmm_bitmap = (uint32_t *)0x200000;
    
    /* Mark all memory as used initially */
    memset(pmm_bitmap, 0xFF, pmm_bitmap_size * sizeof(uint32_t));
    pmm_used_pages = pmm_total_pages;
    
    /* Process memory map if available */
    if (memory_map && entries > 0) {
        for (uint32_t i = 0; i < entries; i++) {
            uint64_t base = ((uint64_t *)memory_map)[i * 3];
            uint64_t length = ((uint64_t *)memory_map)[i * 3 + 1];
            uint32_t type = ((uint32_t *)memory_map)[i * 6 + 4];
            
            if (type == MEMORY_USABLE && base < 0x100000000ULL) {
                uint32_t base32 = (uint32_t)base;
                uint32_t len32 = (uint32_t)MIN(length, 0xFFFFFFFF - base32);
                pmm_mark_region_free(base32, len32);
            }
        }
    } else {
        /* No memory map - assume usable memory from 1MB to memory_size */
        pmm_mark_region_free(MB(1), memory_size - MB(1));
    }
    
    /* Reserve first 4MB for kernel and system structures */
    pmm_mark_region_used(0, MB(4));
    
    /* Find first free page */
    pmm_first_free_page = MB(4) / PAGE_SIZE;
    
    kprintf("PMM: %d MB total, %d KB used, %d KB free\n",
            pmm_memory_size / MB(1),
            (pmm_used_pages * PAGE_SIZE) / KB(1),
            ((pmm_total_pages - pmm_used_pages) * PAGE_SIZE) / KB(1));
}

/*
 * Allocate a single physical page
 */
void *pmm_alloc_page(void) {
    int32_t page = pmm_find_free_page();
    if (page < 0) {
        kerror("PMM: Out of memory!\n");
        return NULL;
    }
    
    pmm_set_bit(page);
    pmm_used_pages++;
    
    void *addr = (void *)(page * PAGE_SIZE);
    memset(addr, 0, PAGE_SIZE);
    
    return addr;
}

/*
 * Allocate multiple contiguous physical pages
 */
void *pmm_alloc_pages(uint32_t count) {
    if (count == 0) return NULL;
    if (count == 1) return pmm_alloc_page();
    
    int32_t start = pmm_find_free_pages(count);
    if (start < 0) {
        kerror("PMM: Cannot allocate %d contiguous pages\n", count);
        return NULL;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        pmm_set_bit(start + i);
    }
    pmm_used_pages += count;
    
    void *addr = (void *)(start * PAGE_SIZE);
    memset(addr, 0, count * PAGE_SIZE);
    
    return addr;
}

/*
 * Free a single physical page
 */
void pmm_free_page(void *addr) {
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    
    if (page < pmm_first_free_page || page >= pmm_total_pages) {
        kwarn("PMM: Attempt to free invalid page 0x%x\n", addr);
        return;
    }
    
    if (!pmm_test_bit(page)) {
        kwarn("PMM: Double free of page 0x%x\n", addr);
        return;
    }
    
    pmm_clear_bit(page);
    pmm_used_pages--;
}

/*
 * Free multiple physical pages
 */
void pmm_free_pages(void *addr, uint32_t count) {
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    
    for (uint32_t i = 0; i < count; i++) {
        pmm_free_page((void *)((page + i) * PAGE_SIZE));
    }
}

/*
 * Allocate DMA-capable memory (below 16MB)
 */
void *pmm_alloc_dma_page(void) {
    uint32_t max_dma_page = MB(16) / PAGE_SIZE;
    
    for (uint32_t page = pmm_first_free_page; page < max_dma_page; page++) {
        if (!pmm_test_bit(page)) {
            pmm_set_bit(page);
            pmm_used_pages++;
            
            void *addr = (void *)(page * PAGE_SIZE);
            memset(addr, 0, PAGE_SIZE);
            return addr;
        }
    }
    
    kerror("PMM: No DMA memory available\n");
    return NULL;
}

/*
 * Get memory statistics
 */
void pmm_get_stats(pmm_stats_t *stats) {
    stats->total_pages = pmm_total_pages;
    stats->used_pages = pmm_used_pages;
    stats->free_pages = pmm_total_pages - pmm_used_pages;
    stats->total_memory = pmm_memory_size;
    stats->used_memory = pmm_used_pages * PAGE_SIZE;
    stats->free_memory = stats->free_pages * PAGE_SIZE;
}
