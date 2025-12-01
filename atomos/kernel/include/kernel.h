/*
 * AtomOS Kernel - Main Header
 * Core kernel definitions and structures
 */

#ifndef _ATOMOS_KERNEL_H
#define _ATOMOS_KERNEL_H

#include "types.h"

/* AtomOS Version */
#define ATOMOS_VERSION_MAJOR    1
#define ATOMOS_VERSION_MINOR    0
#define ATOMOS_VERSION_PATCH    0
#define ATOMOS_VERSION_STRING   "1.0.0"
#define ATOMOS_NAME             "AtomOS"
#define ATOMOS_UI_NAME          "Javier"

/* Boot information passed from bootloader */
typedef struct {
    uint32_t memory_map_entries;
    uint32_t memory_map_addr;
    uint32_t framebuffer_addr;
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t bits_per_pixel;
    uint32_t pitch;
    uint32_t total_memory;
} PACKED boot_info_t;

/* Memory map entry from BIOS E820 */
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attrs;
} PACKED memory_map_entry_t;

/* Memory types */
#define MEMORY_USABLE           1
#define MEMORY_RESERVED         2
#define MEMORY_ACPI_RECLAIMABLE 3
#define MEMORY_ACPI_NVS         4
#define MEMORY_BAD              5

/* Kernel panic */
void panic(const char *message) NORETURN;

/* Kernel logging */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRITICAL
} log_level_t;

void klog(log_level_t level, const char *fmt, ...);

#define kprintf(fmt, ...)   klog(LOG_INFO, fmt, ##__VA_ARGS__)
#define kdebug(fmt, ...)    klog(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define kwarn(fmt, ...)     klog(LOG_WARN, fmt, ##__VA_ARGS__)
#define kerror(fmt, ...)    klog(LOG_ERROR, fmt, ##__VA_ARGS__)

/* Port I/O */
INLINE void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

INLINE uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

INLINE void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

INLINE uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

INLINE void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

INLINE uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* CPU control */
INLINE void cli(void) {
    __asm__ volatile("cli");
}

INLINE void sti(void) {
    __asm__ volatile("sti");
}

INLINE void hlt(void) {
    __asm__ volatile("hlt");
}

INLINE void cpu_relax(void) {
    __asm__ volatile("pause");
}

/* Memory barriers */
INLINE void memory_barrier(void) {
    __asm__ volatile("mfence" ::: "memory");
}

INLINE void read_barrier(void) {
    __asm__ volatile("lfence" ::: "memory");
}

INLINE void write_barrier(void) {
    __asm__ volatile("sfence" ::: "memory");
}

/* String functions (minimal kernel versions) */
void *memset(void *dest, int value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
int memcmp(const void *s1, const void *s2, size_t count);
size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t count);
char *strcat(char *dest, const char *src);
char *strchr(const char *str, int c);
char *strrchr(const char *str, int c);

/* Global boot info */
extern boot_info_t *g_boot_info;

#endif /* _ATOMOS_KERNEL_H */
