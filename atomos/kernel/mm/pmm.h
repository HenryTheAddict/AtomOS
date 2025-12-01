/*
 * AtomOS - Physical Memory Manager
 * Manages physical memory pages using a bitmap allocator
 */

#ifndef _ATOMOS_PMM_H
#define _ATOMOS_PMM_H

#include "../include/types.h"

/* Memory regions */
#define PMM_KERNEL_START    0x100000    /* 1MB - kernel starts here */
#define PMM_MEMORY_END      0x10000000  /* 256MB default max */

/* Statistics */
typedef struct {
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t free_pages;
    uint32_t total_memory;
    uint32_t used_memory;
    uint32_t free_memory;
} pmm_stats_t;

/* Function declarations */
void pmm_init(uint32_t memory_size, uint32_t *memory_map, uint32_t entries);
void *pmm_alloc_page(void);
void *pmm_alloc_pages(uint32_t count);
void pmm_free_page(void *addr);
void pmm_free_pages(void *addr, uint32_t count);

/* Mark memory regions */
void pmm_mark_region_used(uint32_t base, uint32_t size);
void pmm_mark_region_free(uint32_t base, uint32_t size);

/* Statistics */
void pmm_get_stats(pmm_stats_t *stats);

/* DMA memory (below 16MB) */
void *pmm_alloc_dma_page(void);

#endif /* _ATOMOS_PMM_H */
