/*
 * AtomOS - Kernel Heap Allocator
 * Dynamic memory allocation (kmalloc/kfree)
 */

#ifndef _ATOMOS_HEAP_H
#define _ATOMOS_HEAP_H

#include "../include/types.h"

/* Heap configuration */
#define HEAP_INITIAL_SIZE   MB(4)
#define HEAP_MAX_SIZE       MB(256)
#define HEAP_MIN_SIZE       KB(4)

/* Heap block header */
typedef struct heap_block {
    uint32_t magic;         /* Magic number for validation */
    uint32_t size;          /* Size of block (including header) */
    bool free;              /* Is this block free? */
    struct heap_block *next;    /* Next block in list */
    struct heap_block *prev;    /* Previous block in list */
} heap_block_t;

/* Heap structure */
typedef struct {
    uint32_t start;         /* Start of heap */
    uint32_t end;           /* Current end of heap */
    uint32_t max;           /* Maximum heap size */
    heap_block_t *first;    /* First block in free list */
} heap_t;

/* Magic number for validation */
#define HEAP_MAGIC  0xDEADBEEF

/* Function declarations */
void heap_init(void);

/* Kernel memory allocation */
void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size, size_t alignment);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *ptr, size_t size);
void kfree(void *ptr);

/* Heap statistics */
typedef struct {
    uint32_t total_size;
    uint32_t used_size;
    uint32_t free_size;
    uint32_t block_count;
    uint32_t free_block_count;
} heap_stats_t;

void heap_get_stats(heap_stats_t *stats);

#endif /* _ATOMOS_HEAP_H */
