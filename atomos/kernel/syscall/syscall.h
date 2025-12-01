/*
 * AtomOS - System Call Interface
 * Linux-compatible syscall numbers and handlers
 */

#ifndef _ATOMOS_SYSCALL_H
#define _ATOMOS_SYSCALL_H

#include "../include/types.h"
#include "../arch/x86/idt.h"

/* System call numbers (Linux compatible) */
#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_WAITPID     7
#define SYS_CREAT       8
#define SYS_LINK        9
#define SYS_UNLINK      10
#define SYS_EXECVE      11
#define SYS_CHDIR       12
#define SYS_TIME        13
#define SYS_MKNOD       14
#define SYS_CHMOD       15
#define SYS_LSEEK       19
#define SYS_GETPID      20
#define SYS_MOUNT       21
#define SYS_UMOUNT      22
#define SYS_SETUID      23
#define SYS_GETUID      24
#define SYS_STIME       25
#define SYS_ALARM       27
#define SYS_PAUSE       29
#define SYS_UTIME       30
#define SYS_ACCESS      33
#define SYS_NICE        34
#define SYS_SYNC        36
#define SYS_KILL        37
#define SYS_RENAME      38
#define SYS_MKDIR       39
#define SYS_RMDIR       40
#define SYS_DUP         41
#define SYS_PIPE        42
#define SYS_TIMES       43
#define SYS_BRK         45
#define SYS_SETGID      46
#define SYS_GETGID      47
#define SYS_SIGNAL      48
#define SYS_GETEUID     49
#define SYS_GETEGID     50
#define SYS_IOCTL       54
#define SYS_FCNTL       55
#define SYS_SETPGID     57
#define SYS_UMASK       60
#define SYS_CHROOT      61
#define SYS_DUP2        63
#define SYS_GETPPID     64
#define SYS_GETPGRP     65
#define SYS_SETSID      66
#define SYS_SIGACTION   67
#define SYS_SGETMASK    68
#define SYS_SSETMASK    69
#define SYS_SIGSUSPEND  72
#define SYS_SIGPENDING  73
#define SYS_SETHOSTNAME 74
#define SYS_GETTIMEOFDAY 78
#define SYS_SETTIMEOFDAY 79
#define SYS_GETGROUPS   80
#define SYS_SETGROUPS   81
#define SYS_SELECT      82
#define SYS_SYMLINK     83
#define SYS_READLINK    85
#define SYS_MMAP        90
#define SYS_MUNMAP      91
#define SYS_TRUNCATE    92
#define SYS_FTRUNCATE   93
#define SYS_FCHMOD      94
#define SYS_FCHOWN      95
#define SYS_GETPRIORITY 96
#define SYS_SETPRIORITY 97
#define SYS_STATFS      99
#define SYS_FSTATFS     100
#define SYS_SOCKETCALL  102
#define SYS_SYSLOG      103
#define SYS_SETITIMER   104
#define SYS_GETITIMER   105
#define SYS_STAT        106
#define SYS_LSTAT       107
#define SYS_FSTAT       108
#define SYS_CLONE       120
#define SYS_UNAME       122
#define SYS_MPROTECT    125
#define SYS_GETDENTS    141
#define SYS_MSYNC       144
#define SYS_READV       145
#define SYS_WRITEV      146
#define SYS_NANOSLEEP   162
#define SYS_GETCWD      183
#define SYS_TKILL       238

/* AtomOS specific syscalls (start at 400) */
#define SYS_ATOMOS_DEBUG    400
#define SYS_ATOMOS_WINDOW   401
#define SYS_ATOMOS_GRAPHICS 402
#define SYS_ATOMOS_EVENT    403

/* Syscall handler function type */
typedef int32_t (*syscall_handler_t)(uint32_t, uint32_t, uint32_t, 
                                     uint32_t, uint32_t, uint32_t);

/* Initialize syscall interface */
void syscall_init(void);

/* Register a syscall handler */
void syscall_register(uint32_t num, syscall_handler_t handler);

/* Syscall dispatcher (called from interrupt handler) */
void syscall_handler(registers_t *regs);

#endif /* _ATOMOS_SYSCALL_H */
