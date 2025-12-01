/*
 * AtomOS - Linux Compatibility Layer
 * Provides Linux binary compatibility through syscall emulation
 */

#ifndef _ATOMOS_LINUX_COMPAT_H
#define _ATOMOS_LINUX_COMPAT_H

#include "../../kernel/include/types.h"

/* Linux ELF structures */
#define ELF_MAGIC       0x464C457F  /* 0x7F 'E' 'L' 'F' */

/* ELF header */
typedef struct {
    uint32_t magic;
    uint8_t  class;         /* 1 = 32-bit, 2 = 64-bit */
    uint8_t  data;          /* 1 = little endian, 2 = big endian */
    uint8_t  version;
    uint8_t  osabi;
    uint8_t  abiversion;
    uint8_t  pad[7];
    uint16_t type;          /* 1 = relocatable, 2 = executable, 3 = shared */
    uint16_t machine;       /* 3 = x86, 0x3E = x86_64 */
    uint32_t version2;
    uint32_t entry;         /* Entry point */
    uint32_t phoff;         /* Program header offset */
    uint32_t shoff;         /* Section header offset */
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} PACKED elf32_header_t;

/* ELF program header */
typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} PACKED elf32_phdr_t;

/* Program header types */
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6
#define PT_TLS      7

/* Program header flags */
#define PF_X        0x1     /* Executable */
#define PF_W        0x2     /* Writable */
#define PF_R        0x4     /* Readable */

/* Linux structures */
typedef struct {
    unsigned long rlim_cur;
    unsigned long rlim_max;
} linux_rlimit_t;

typedef struct {
    int tv_sec;
    int tv_usec;
} linux_timeval_t;

typedef struct {
    int tz_minuteswest;
    int tz_dsttime;
} linux_timezone_t;

/* Linux signal definitions */
#define LINUX_SIGHUP    1
#define LINUX_SIGINT    2
#define LINUX_SIGQUIT   3
#define LINUX_SIGILL    4
#define LINUX_SIGTRAP   5
#define LINUX_SIGABRT   6
#define LINUX_SIGBUS    7
#define LINUX_SIGFPE    8
#define LINUX_SIGKILL   9
#define LINUX_SIGUSR1   10
#define LINUX_SIGSEGV   11
#define LINUX_SIGUSR2   12
#define LINUX_SIGPIPE   13
#define LINUX_SIGALRM   14
#define LINUX_SIGTERM   15
#define LINUX_SIGCHLD   17
#define LINUX_SIGCONT   18
#define LINUX_SIGSTOP   19
#define LINUX_SIGTSTP   20

/* Function declarations */
void linux_compat_init(void);
int linux_load_elf(const char *path, uint32_t *entry_point);
int linux_exec(const char *path, char *const argv[], char *const envp[]);

/* Syscall handlers */
int linux_sys_read(int fd, void *buf, size_t count);
int linux_sys_write(int fd, const void *buf, size_t count);
int linux_sys_open(const char *path, int flags, int mode);
int linux_sys_close(int fd);
int linux_sys_stat(const char *path, void *buf);
int linux_sys_fstat(int fd, void *buf);
int linux_sys_lseek(int fd, off_t offset, int whence);
int linux_sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int linux_sys_munmap(void *addr, size_t len);
int linux_sys_brk(void *addr);
int linux_sys_ioctl(int fd, unsigned long request, void *arg);
int linux_sys_getpid(void);
int linux_sys_fork(void);
int linux_sys_execve(const char *path, char *const argv[], char *const envp[]);
int linux_sys_exit(int status);
int linux_sys_waitpid(int pid, int *status, int options);
int linux_sys_kill(int pid, int sig);
int linux_sys_uname(void *buf);
int linux_sys_getcwd(char *buf, size_t size);
int linux_sys_chdir(const char *path);
int linux_sys_getdents(int fd, void *dirp, unsigned int count);
int linux_sys_gettimeofday(linux_timeval_t *tv, linux_timezone_t *tz);
int linux_sys_nanosleep(const void *req, void *rem);
int linux_sys_socket(int domain, int type, int protocol);
int linux_sys_connect(int fd, const void *addr, int addrlen);
int linux_sys_accept(int fd, void *addr, int *addrlen);
int linux_sys_sendto(int fd, const void *buf, size_t len, int flags,
                     const void *dest_addr, int addrlen);
int linux_sys_recvfrom(int fd, void *buf, size_t len, int flags,
                       void *src_addr, int *addrlen);
int linux_sys_bind(int fd, const void *addr, int addrlen);
int linux_sys_listen(int fd, int backlog);
int linux_sys_clone(unsigned long flags, void *stack, int *ptid, int *ctid, void *tls);

#endif /* _ATOMOS_LINUX_COMPAT_H */
