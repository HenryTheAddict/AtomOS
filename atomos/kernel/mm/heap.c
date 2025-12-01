/*
 * AtomOS - Kernel Heap Implementation
 * First-fit allocator with coalescing
 */

#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "../include/kernel.h"

/* Global kernel heap */
static heap_t kernel_heap;

/* Minimum allocation block size */
#define MIN_BLOCK_SIZE  (sizeof(heap_block_t) + 16)

/*
 * Expand the heap
 */
static bool heap_expand(size_t size) {
    size = ALIGN_UP(size, PAGE_SIZE);
    
    if (kernel_heap.end + size > kernel_heap.max) {
        kerror("Heap: Cannot expand beyond maximum size\n");
        return false;
    }
    
    /* Allocate new pages */
    uint32_t pages = size / PAGE_SIZE;
    for (uint32_t i = 0; i < pages; i++) {
        void *phys = pmm_alloc_page();
        if (!phys) {
            kerror("Heap: Out of physical memory during expansion\n");
            return false;
        }
        vmm_map_page(vmm_get_kernel_directory(), 
                     kernel_heap.end + i * PAGE_SIZE,
                     (uint32_t)phys, 
                     PTE_WRITABLE);
    }
    
    /* Create new free block at the end */
    heap_block_t *new_block = (heap_block_t *)kernel_heap.end;
    new_block->magic = HEAP_MAGIC;
    new_block->size = size;
    new_block->free = true;
    new_block->next = NULL;
    
    /* Link to existing blocks */
    if (kernel_heap.first == NULL) {
        new_block->prev = NULL;
        kernel_heap.first = new_block;
    } else {
        /* Find last block */
        heap_block_t *last = kernel_heap.first;
        while (last->next) {
            last = last->next;
        }
        last->next = new_block;
        new_block->prev = last;
        
        /* Coalesce with previous if it's free */
        if (last->free) {
            last->size += new_block->size;
            last->next = NULL;
        }
    }
    
    kernel_heap.end += size;
    return true;
}

/*
 * Split a block if it's too large
 */
static void heap_split_block(heap_block_t *block, size_t size) {
    if (block->size - size < MIN_BLOCK_SIZE) {
        return;
    }
    
    heap_block_t *new_block = (heap_block_t *)((uint32_t)block + size);
    new_block->magic = HEAP_MAGIC;
    new_block->size = block->size - size;
    new_block->free = true;
    new_block->next = block->next;
    new_block->prev = block;
    
    if (block->next) {
        block->next->prev = new_block;
    }
    
    block->size = size;
    block->next = new_block;
}

/*
 * Coalesce adjacent free blocks
 */
static void heap_coalesce(heap_block_t *block) {
    /* Coalesce with next block */
    while (block->next && block->next->free) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    /* Coalesce with previous block */
    while (block->prev && block->prev->free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

/*
 * Find a suitable free block
 */
static heap_block_t *heap_find_block(size_t size) {
    heap_block_t *block = kernel_heap.first;
    
    while (block) {
        if (block->free && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    
    return NULL;
}

/*
 * Initialize the kernel heap
 */
void heap_init(void) {
    /* Heap starts at 4MB mark */
    kernel_heap.start = MB(4);
    kernel_heap.end = MB(4);
    kernel_heap.max = MB(64);   /* 64MB max heap */
    kernel_heap.first = NULL;
    
    /* Allocate initial heap */
    if (!heap_expand(HEAP_INITIAL_SIZE)) {
        panic("Failed to initialize kernel heap");
    }
    
    kprintf("Heap initialized: %d KB\n", HEAP_INITIAL_SIZE / KB(1));
}

/*
 * Allocate memory from the heap
 */
void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    /* Add header size and align */
    size = ALIGN_UP(size + sizeof(heap_block_t), 16);
    
    /* Find a free block */
    heap_block_t *block = heap_find_block(size);
    
    /* Expand heap if needed */
    if (!block) {
        size_t expand_size = MAX(size, HEAP_INITIAL_SIZE);
        if (!heap_expand(expand_size)) {
            kerror("kmalloc: Out of memory (requested %d bytes)\n", size);
            return NULL;
        }
        block = heap_find_block(size);
        if (!block) {
            return NULL;
        }
    }
    
    /* Split block if too large */
    heap_split_block(block, size);
    
    /* Mark as used */
    block->free = false;
    
    /* Return pointer after header */
    return (void *)((uint32_t)block + sizeof(heap_block_t));
}

/*
 * Allocate aligned memory
 */
void *kmalloc_aligned(size_t size, size_t alignment) {
    /* Allocate extra space for alignment */
    void *ptr = kmalloc(size + alignment);
    if (!ptr) {
        return NULL;
    }
    
    /* Align the pointer */
    uintptr_t aligned = ALIGN_UP((uintptr_t)ptr, alignment);
    
    /* Store original pointer for freeing */
    if (aligned != (uintptr_t)ptr) {
        *((void **)(aligned - sizeof(void *))) = ptr;
    }
    
    return (void *)aligned;
}

/*
 * Allocate and zero memory
 */
void *kcalloc(size_t count, size_t size) {
    size_t total = count * size;
    void *ptr = kmalloc(total);
    
    if (ptr) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

/*
 * Reallocate memory
 */
void *krealloc(void *ptr, size_t size) {
    if (!ptr) {
        return kmalloc(size);
    }
    
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    /* Get block header */
    heap_block_t *block = (heap_block_t *)((uint32_t)ptr - sizeof(heap_block_t));
    
    if (block->magic != HEAP_MAGIC) {
        kerror("krealloc: Invalid block magic\n");
        return NULL;
    }
    
    size_t old_size = block->size - sizeof(heap_block_t);
    
    /* If new size fits in current block, return same pointer */
    if (size <= old_size) {
        return ptr;
    }
    
    /* Allocate new block and copy */
    void *new_ptr = kmalloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    memcpy(new_ptr, ptr, old_size);
    kfree(ptr);
    
    return new_ptr;
}

/*
 * Free allocated memory
 */
void kfree(void *ptr) {
    if (!ptr) {
        return;
    }
    
    /* Get block header */
    heap_block_t *block = (heap_block_t *)((uint32_t)ptr - sizeof(heap_block_t));
    
    if (block->magic != HEAP_MAGIC) {
        kerror("kfree: Invalid block magic at 0x%x\n", ptr);
        return;
    }
    
    if (block->free) {
        kwarn("kfree: Double free at 0x%x\n", ptr);
        return;
    }
    
    /* Mark as free */
    block->free = true;
    
    /* Coalesce with neighbors */
    heap_coalesce(block);
}

/*
 * Get heap statistics
 */
void heap_get_stats(heap_stats_t *stats) {
    stats->total_size = kernel_heap.end - kernel_heap.start;
    stats->used_size = 0;
    stats->free_size = 0;
    stats->block_count = 0;
    stats->free_block_count = 0;
    
    heap_block_t *block = kernel_heap.first;
    while (block) {
        stats->block_count++;
        if (block->free) {
            stats->free_size += block->size;
            stats->free_block_count++;
        } else {
            stats->used_size += block->size;
        }
        block = block->next;
    }
}
