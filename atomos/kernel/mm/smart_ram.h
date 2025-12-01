/*
 * AtomOS Smart RAM Management
 * Intelligent memory allocation with compression and defragmentation
 */

#ifndef _ATOMOS_SMART_RAM_H
#define _ATOMOS_SMART_RAM_H

#include "../include/types.h"

/* Memory pressure levels */
typedef enum {
    MEM_PRESSURE_LOW,       /* < 50% used */
    MEM_PRESSURE_MEDIUM,    /* 50-75% used */
    MEM_PRESSURE_HIGH,      /* 75-90% used */
    MEM_PRESSURE_CRITICAL   /* > 90% used */
} mem_pressure_t;

/* Memory region types */
typedef enum {
    MEM_REGION_KERNEL,      /* Kernel code/data */
    MEM_REGION_USER,        /* User processes */
    MEM_REGION_CACHE,       /* Disk cache */
    MEM_REGION_BUFFER,      /* I/O buffers */
    MEM_REGION_GRAPHICS,    /* Framebuffer/3D */
    MEM_REGION_TEMP         /* Temporary allocations */
} mem_region_type_t;

/* Memory allocation hints */
typedef enum {
    ALLOC_NORMAL = 0,       /* Normal allocation */
    ALLOC_FAST,             /* Fast path, no cleanup */
    ALLOC_COMPACT,          /* Can be compacted */
    ALLOC_LOCKED,           /* Cannot be moved/swapped */
    ALLOC_DMA,              /* DMA-capable memory */
    ALLOC_LOWMEM,           /* Low memory preference */
    ALLOC_HIGHMEM,          /* High memory OK */
    ALLOC_ZERO              /* Zero-initialized */
} alloc_hint_t;

/* Memory statistics */
typedef struct {
    uint32_t total_ram;
    uint32_t free_ram;
    uint32_t cached_ram;
    uint32_t buffer_ram;
    uint32_t kernel_ram;
    uint32_t user_ram;
    uint32_t graphics_ram;
    uint32_t peak_usage;
    uint32_t alloc_count;
    uint32_t free_count;
    uint32_t compactions;
    uint32_t oom_events;
    mem_pressure_t pressure;
} smart_ram_stats_t;

/* Memory block header (for smart allocator) */
typedef struct smart_block {
    uint32_t magic;
    uint32_t size;
    alloc_hint_t hint;
    mem_region_type_t region;
    uint32_t age;           /* Ticks since last access */
    bool in_use;
    bool compactable;
    struct smart_block *next;
    struct smart_block *prev;
} smart_block_t;

/* Page cache entry */
typedef struct {
    uint32_t page_addr;
    uint32_t access_count;
    uint64_t last_access;
    bool dirty;
    bool locked;
} cache_entry_t;

/* Initialize smart RAM management */
void smart_ram_init(void);

/* Smart allocation */
void *smart_alloc(size_t size, alloc_hint_t hint);
void *smart_alloc_region(size_t size, mem_region_type_t region);
void smart_free(void *ptr);
void *smart_realloc(void *ptr, size_t new_size);

/* Memory optimization */
void smart_ram_compact(void);
void smart_ram_cleanup(void);
void smart_ram_flush_caches(void);

/* Cache management */
void *smart_cache_alloc(size_t size);
void smart_cache_free(void *ptr);
void smart_cache_flush(void);
uint32_t smart_cache_size(void);

/* Memory pressure handling */
mem_pressure_t smart_ram_pressure(void);
void smart_ram_reclaim(size_t bytes);
bool smart_ram_available(size_t bytes);

/* Statistics */
void smart_ram_get_stats(smart_ram_stats_t *stats);
void smart_ram_print_stats(void);

/* Memory profiling */
void smart_ram_mark_access(void *ptr);
void smart_ram_age_blocks(void);

/* Low-level optimization */
void smart_ram_prefetch(void *ptr, size_t size);
void smart_ram_writeback(void *ptr, size_t size);

/* OOM (Out of Memory) handling */
typedef void (*oom_handler_t)(size_t requested);
void smart_ram_set_oom_handler(oom_handler_t handler);

#endif /* _ATOMOS_SMART_RAM_H */
