/*
 * AtomOS - Core Type Definitions
 * Fundamental data types for the operating system
 */

#ifndef _ATOMOS_TYPES_H
#define _ATOMOS_TYPES_H

/* Standard integer types */
typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
typedef unsigned long long  uint64_t;
typedef signed long long    int64_t;

/* Size types */
typedef uint32_t            size_t;
typedef int32_t             ssize_t;
typedef int32_t             ptrdiff_t;
typedef uint32_t            uintptr_t;
typedef int32_t             intptr_t;

/* Boolean type */
typedef uint8_t             bool;
#define true                1
#define false               0

/* NULL pointer */
#define NULL                ((void*)0)

/* Useful macros */
#define PACKED              __attribute__((packed))
#define ALIGNED(x)          __attribute__((aligned(x)))
#define NORETURN            __attribute__((noreturn))
#define UNUSED              __attribute__((unused))
#define INLINE              static inline

/* Memory size helpers */
#define KB(x)               ((x) * 1024UL)
#define MB(x)               ((x) * 1024UL * 1024UL)
#define GB(x)               ((x) * 1024UL * 1024UL * 1024UL)

/* Bit manipulation */
#define BIT(x)              (1UL << (x))
#define SET_BIT(v, b)       ((v) |= BIT(b))
#define CLEAR_BIT(v, b)     ((v) &= ~BIT(b))
#define TEST_BIT(v, b)      ((v) & BIT(b))

/* Min/Max */
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

/* Array size */
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))

/* Alignment */
#define ALIGN_UP(x, a)      (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))

/* Page size */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           (PAGE_SIZE - 1)

/* Return codes */
typedef int32_t             status_t;
#define STATUS_OK           0
#define STATUS_ERROR        (-1)
#define STATUS_NOMEM        (-2)
#define STATUS_INVALID      (-3)
#define STATUS_NOTFOUND     (-4)
#define STATUS_BUSY         (-5)
#define STATUS_TIMEOUT      (-6)
#define STATUS_PERM         (-7)

/* File descriptors */
typedef int32_t             fd_t;

/* Process ID */
typedef uint32_t            pid_t;

/* Thread ID */
typedef uint32_t            tid_t;

/* Time types */
typedef uint64_t            time_t;
typedef uint32_t            useconds_t;

/* Offset type */
typedef int64_t             off_t;

/* Mode type */
typedef uint32_t            mode_t;

/* User/Group IDs */
typedef uint32_t            uid_t;
typedef uint32_t            gid_t;

/* Device type */
typedef uint32_t            dev_t;

/* Inode number */
typedef uint32_t            ino_t;

/* Block count */
typedef uint32_t            blkcnt_t;

/* Block size */
typedef uint32_t            blksize_t;

/* Network types */
typedef uint32_t            in_addr_t;
typedef uint16_t            in_port_t;

#endif /* _ATOMOS_TYPES_H */
