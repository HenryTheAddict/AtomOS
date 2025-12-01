/*
 * AtomOS Smart RAM Management Implementation
 * Adaptive memory management for optimal performance
 */

#include "smart_ram.h"
#include "pmm.h"
#include "heap.h"
#include "../include/kernel.h"
#include "../drivers/timer.h"

/* Smart RAM configuration */
#define SMART_BLOCK_MAGIC   0x534D5254  /* "SMRT" */
#define CACHE_MAX_SIZE      MB(16)
#define AGING_INTERVAL      100         /* Ticks between aging */
#define COMPACT_THRESHOLD   75          /* % fragmentation to trigger */

/* Global state */
static smart_block_t *block_list = NULL;
static uint32_t total_allocated = 0;
static uint32_t peak_allocated = 0;
static uint32_t alloc_count = 0;
static uint32_t free_count = 0;
static uint32_t compact_count = 0;
static uint32_t oom_count = 0;
static uint64_t last_aging = 0;
static oom_handler_t oom_handler = NULL;

/* Cache state */
static uint32_t cache_used = 0;
static smart_block_t *cache_list = NULL;

/*
 * Initialize smart RAM management
 */
void smart_ram_init(void) {
    block_list = NULL;
    cache_list = NULL;
    total_allocated = 0;
    peak_allocated = 0;
    alloc_count = 0;
    free_count = 0;
    compact_count = 0;
    oom_count = 0;
    cache_used = 0;
    last_aging = timer_get_ticks();
    
    kprintf("Smart RAM management initialized\n");
}

/*
 * Get current memory pressure level
 */
mem_pressure_t smart_ram_pressure(void) {
    pmm_stats_t stats;
    pmm_get_stats(&stats);
    
    uint32_t used_percent = (stats.used_memory * 100) / stats.total_memory;
    
    if (used_percent < 50) return MEM_PRESSURE_LOW;
    if (used_percent < 75) return MEM_PRESSURE_MEDIUM;
    if (used_percent < 90) return MEM_PRESSURE_HIGH;
    return MEM_PRESSURE_CRITICAL;
}

/*
 * Check if memory is available
 */
bool smart_ram_available(size_t bytes) {
    pmm_stats_t stats;
    pmm_get_stats(&stats);
    return stats.free_memory >= bytes;
}

/*
 * Age all memory blocks (called periodically)
 */
void smart_ram_age_blocks(void) {
    uint64_t now = timer_get_ticks();
    if (now - last_aging < AGING_INTERVAL) return;
    last_aging = now;
    
    smart_block_t *block = block_list;
    while (block) {
        if (block->in_use) {
            block->age++;
        }
        block = block->next;
    }
}

/*
 * Find best fit block for allocation
 */
static smart_block_t *find_best_fit(size_t size) {
    smart_block_t *best = NULL;
    size_t best_size = (size_t)-1;  /* Maximum size_t value */
    
    smart_block_t *block = block_list;
    while (block) {
        if (!block->in_use && block->size >= size) {
            if (block->size < best_size) {
                best = block;
                best_size = block->size;
                
                /* Perfect fit */
                if (block->size == size) break;
            }
        }
        block = block->next;
    }
    
    return best;
}

/*
 * Smart allocation with hints
 */
void *smart_alloc(size_t size, alloc_hint_t hint) {
    if (size == 0) return NULL;
    
    /* Round up to 16-byte alignment */
    size = ALIGN_UP(size, 16);
    size_t total_size = size + sizeof(smart_block_t);
    
    /* Check memory pressure and try to reclaim if needed */
    mem_pressure_t pressure = smart_ram_pressure();
    if (pressure >= MEM_PRESSURE_HIGH) {
        smart_ram_cleanup();
        if (pressure == MEM_PRESSURE_CRITICAL) {
            smart_ram_flush_caches();
        }
    }
    
    /* Try to find existing free block */
    smart_block_t *block = find_best_fit(size);
    
    if (!block) {
        /* Allocate new block */
        uint32_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
        void *mem = pmm_alloc_pages(pages);
        
        if (!mem) {
            /* OOM - try more aggressive reclamation */
            smart_ram_reclaim(total_size);
            mem = pmm_alloc_pages(pages);
            
            if (!mem) {
                oom_count++;
                if (oom_handler) {
                    oom_handler(size);
                }
                kerror("Smart RAM: Out of memory (requested %d bytes)\n", size);
                return NULL;
            }
        }
        
        block = (smart_block_t *)mem;
        block->size = pages * PAGE_SIZE - sizeof(smart_block_t);
        
        /* Add to block list */
        block->next = block_list;
        block->prev = NULL;
        if (block_list) block_list->prev = block;
        block_list = block;
    }
    
    /* Initialize block */
    block->magic = SMART_BLOCK_MAGIC;
    block->hint = hint;
    block->region = MEM_REGION_USER;
    block->age = 0;
    block->in_use = true;
    block->compactable = (hint == ALLOC_COMPACT);
    
    /* Update statistics */
    total_allocated += size;
    alloc_count++;
    if (total_allocated > peak_allocated) {
        peak_allocated = total_allocated;
    }
    
    /* Zero if requested */
    void *ptr = (void *)((uint8_t *)block + sizeof(smart_block_t));
    if (hint == ALLOC_ZERO) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

/*
 * Allocate for specific region
 */
void *smart_alloc_region(size_t size, mem_region_type_t region) {
    void *ptr = smart_alloc(size, ALLOC_NORMAL);
    if (ptr) {
        smart_block_t *block = (smart_block_t *)((uint8_t *)ptr - sizeof(smart_block_t));
        block->region = region;
    }
    return ptr;
}

/*
 * Smart free
 */
void smart_free(void *ptr) {
    if (!ptr) return;
    
    smart_block_t *block = (smart_block_t *)((uint8_t *)ptr - sizeof(smart_block_t));
    
    if (block->magic != SMART_BLOCK_MAGIC) {
        kerror("Smart RAM: Invalid free (corrupt block)\n");
        return;
    }
    
    if (!block->in_use) {
        kwarn("Smart RAM: Double free detected\n");
        return;
    }
    
    /* Mark as free */
    block->in_use = false;
    block->age = 0;
    
    /* Update statistics */
    total_allocated -= block->size;
    free_count++;
    
    /* Coalesce with adjacent free blocks */
    if (block->next && !block->next->in_use) {
        block->size += block->next->size + sizeof(smart_block_t);
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }
    if (block->prev && !block->prev->in_use) {
        block->prev->size += block->size + sizeof(smart_block_t);
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}

/*
 * Reallocate memory
 */
void *smart_realloc(void *ptr, size_t new_size) {
    if (!ptr) return smart_alloc(new_size, ALLOC_NORMAL);
    if (new_size == 0) {
        smart_free(ptr);
        return NULL;
    }
    
    smart_block_t *block = (smart_block_t *)((uint8_t *)ptr - sizeof(smart_block_t));
    
    /* If current block is big enough, reuse it */
    if (block->size >= new_size) {
        return ptr;
    }
    
    /* Allocate new block and copy */
    void *new_ptr = smart_alloc(new_size, block->hint);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        smart_free(ptr);
    }
    
    return new_ptr;
}

/*
 * Compact memory (move blocks to reduce fragmentation)
 */
void smart_ram_compact(void) {
    /* Only compact compactable blocks */
    smart_block_t *src = block_list;
    
    while (src) {
        if (src->in_use && src->compactable) {
            /* Find a lower free block to move into */
            smart_block_t *target = block_list;
            while (target && target != src) {
                if (!target->in_use && target->size >= src->size) {
                    /* Move block */
                    void *src_data = (void *)((uint8_t *)src + sizeof(smart_block_t));
                    void *dst_data = (void *)((uint8_t *)target + sizeof(smart_block_t));
                    
                    memcpy(dst_data, src_data, src->size);
                    
                    /* Swap metadata */
                    target->in_use = true;
                    target->hint = src->hint;
                    target->region = src->region;
                    target->age = src->age;
                    target->compactable = src->compactable;
                    
                    src->in_use = false;
                    
                    compact_count++;
                    break;
                }
                target = target->next;
            }
        }
        src = src->next;
    }
}

/*
 * Cleanup old/unused allocations
 */
void smart_ram_cleanup(void) {
    /* Age blocks and identify candidates for cleanup */
    smart_ram_age_blocks();
    
    /* Free very old cache blocks */
    smart_block_t *block = cache_list;
    while (block) {
        smart_block_t *next = block->next;
        if (block->age > 1000) {  /* Very old */
            smart_cache_free((void *)((uint8_t *)block + sizeof(smart_block_t)));
        }
        block = next;
    }
}

/*
 * Flush all caches
 */
void smart_ram_flush_caches(void) {
    while (cache_list) {
        smart_block_t *next = cache_list->next;
        pmm_free_pages(cache_list, 
                       (cache_list->size + sizeof(smart_block_t) + PAGE_SIZE - 1) / PAGE_SIZE);
        cache_list = next;
    }
    cache_used = 0;
    kprintf("Smart RAM: Flushed all caches\n");
}

/*
 * Reclaim memory aggressively
 */
void smart_ram_reclaim(size_t bytes) {
    kprintf("Smart RAM: Reclaiming %d bytes\n", bytes);
    
    /* First, compact */
    smart_ram_compact();
    
    /* Then flush caches */
    smart_ram_flush_caches();
    
    /* Free old unused blocks */
    smart_block_t *block = block_list;
    size_t reclaimed = 0;
    
    while (block && reclaimed < bytes) {
        smart_block_t *next = block->next;
        if (!block->in_use) {
            /* Remove from list */
            if (block->prev) block->prev->next = block->next;
            else block_list = block->next;
            if (block->next) block->next->prev = block->prev;
            
            reclaimed += block->size + sizeof(smart_block_t);
            pmm_free_pages(block, 
                          (block->size + sizeof(smart_block_t) + PAGE_SIZE - 1) / PAGE_SIZE);
        }
        block = next;
    }
}

/*
 * Flush cache (forward declaration used below)
 */
void smart_cache_flush(void) {
    while (cache_list) {
        smart_block_t *next = cache_list->next;
        if (cache_list->in_use) {
            cache_list->in_use = false;
            total_allocated -= cache_list->size;
            free_count++;
        }
        cache_list = next;
    }
    cache_used = 0;
}

/*
 * Cache allocation
 */
void *smart_cache_alloc(size_t size) {
    if (cache_used + size > CACHE_MAX_SIZE) {
        smart_cache_flush();
    }
    
    void *ptr = smart_alloc(size, ALLOC_COMPACT);
    if (ptr) {
        smart_block_t *block = (smart_block_t *)((uint8_t *)ptr - sizeof(smart_block_t));
        block->region = MEM_REGION_CACHE;
        
        /* Add to cache list */
        block->next = cache_list;
        if (cache_list) cache_list->prev = block;
        cache_list = block;
        
        cache_used += size;
    }
    
    return ptr;
}

/*
 * Cache free
 */
void smart_cache_free(void *ptr) {
    if (!ptr) return;
    
    smart_block_t *block = (smart_block_t *)((uint8_t *)ptr - sizeof(smart_block_t));
    cache_used -= block->size;
    
    /* Remove from cache list */
    if (block->prev) block->prev->next = block->next;
    else if (cache_list == block) cache_list = block->next;
    if (block->next) block->next->prev = block->prev;
    
    smart_free(ptr);
}

/*
 * Get cache size
 */
uint32_t smart_cache_size(void) {
    return cache_used;
}

/*
 * Mark memory access (for aging)
 */
void smart_ram_mark_access(void *ptr) {
    if (!ptr) return;
    
    smart_block_t *block = (smart_block_t *)((uint8_t *)ptr - sizeof(smart_block_t));
    if (block->magic == SMART_BLOCK_MAGIC) {
        block->age = 0;  /* Reset age on access */
    }
}

/*
 * Get statistics
 */
void smart_ram_get_stats(smart_ram_stats_t *stats) {
    pmm_stats_t pmm_stats;
    pmm_get_stats(&pmm_stats);
    
    stats->total_ram = pmm_stats.total_memory;
    stats->free_ram = pmm_stats.free_memory;
    stats->cached_ram = cache_used;
    stats->buffer_ram = 0;
    stats->kernel_ram = 0;
    stats->user_ram = 0;
    stats->graphics_ram = 0;
    
    /* Count by region */
    smart_block_t *block = block_list;
    while (block) {
        if (block->in_use) {
            switch (block->region) {
                case MEM_REGION_KERNEL: stats->kernel_ram += block->size; break;
                case MEM_REGION_USER: stats->user_ram += block->size; break;
                case MEM_REGION_BUFFER: stats->buffer_ram += block->size; break;
                case MEM_REGION_GRAPHICS: stats->graphics_ram += block->size; break;
                default: break;
            }
        }
        block = block->next;
    }
    
    stats->peak_usage = peak_allocated;
    stats->alloc_count = alloc_count;
    stats->free_count = free_count;
    stats->compactions = compact_count;
    stats->oom_events = oom_count;
    stats->pressure = smart_ram_pressure();
}

/*
 * Print statistics
 */
void smart_ram_print_stats(void) {
    smart_ram_stats_t stats;
    smart_ram_get_stats(&stats);
    
    kprintf("=== Smart RAM Statistics ===\n");
    kprintf("Total:    %d KB\n", stats.total_ram / 1024);
    kprintf("Free:     %d KB\n", stats.free_ram / 1024);
    kprintf("Cached:   %d KB\n", stats.cached_ram / 1024);
    kprintf("Kernel:   %d KB\n", stats.kernel_ram / 1024);
    kprintf("User:     %d KB\n", stats.user_ram / 1024);
    kprintf("Graphics: %d KB\n", stats.graphics_ram / 1024);
    kprintf("Peak:     %d KB\n", stats.peak_usage / 1024);
    kprintf("Allocs:   %d\n", stats.alloc_count);
    kprintf("Frees:    %d\n", stats.free_count);
    kprintf("Compacts: %d\n", stats.compactions);
    kprintf("OOM:      %d\n", stats.oom_events);
    
    const char *pressure_str[] = {"Low", "Medium", "High", "Critical"};
    kprintf("Pressure: %s\n", pressure_str[stats.pressure]);
}

/*
 * Set OOM handler
 */
void smart_ram_set_oom_handler(oom_handler_t handler) {
    oom_handler = handler;
}
