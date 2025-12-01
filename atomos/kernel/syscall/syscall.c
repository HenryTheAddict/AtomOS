/*
 * AtomOS - System Call Implementation
 * Provides Linux-compatible syscalls for application compatibility
 */

#include "syscall.h"
#include "../include/kernel.h"
#include "../process/process.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../mm/heap.h"
#include "../drivers/timer.h"
#include "../fs/vfs.h"

/* Syscall table */
#define MAX_SYSCALLS 512
static syscall_handler_t syscall_table[MAX_SYSCALLS];

/*
 * SYS_EXIT - Exit process
 */
static int32_t sys_exit(uint32_t code, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                        uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    process_exit(code);
    return 0;
}

/*
 * SYS_FORK - Fork process
 */
static int32_t sys_fork(uint32_t a1 UNUSED, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                        uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    return process_fork();
}

/*
 * SYS_READ - Read from file
 */
static int32_t sys_read(uint32_t fd, uint32_t buf, uint32_t count,
                        uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    return vfs_read(fd, (void *)buf, count);
}

/*
 * SYS_WRITE - Write to file
 */
static int32_t sys_write(uint32_t fd, uint32_t buf, uint32_t count,
                         uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    return vfs_write(fd, (const void *)buf, count);
}

/*
 * SYS_OPEN - Open file
 */
static int32_t sys_open(uint32_t pathname, uint32_t flags, uint32_t mode,
                        uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    return vfs_open((const char *)pathname, flags, mode);
}

/*
 * SYS_CLOSE - Close file
 */
static int32_t sys_close(uint32_t fd, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                         uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    return vfs_close(fd);
}

/*
 * SYS_WAITPID - Wait for child process
 */
static int32_t sys_waitpid(uint32_t pid, uint32_t status, uint32_t options,
                           uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    return process_waitpid(pid, (int *)status, options);
}

/*
 * SYS_EXECVE - Execute program
 */
static int32_t sys_execve(uint32_t path, uint32_t argv, uint32_t envp,
                          uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    return process_exec((const char *)path, (char *const *)argv, (char *const *)envp);
}

/*
 * SYS_GETPID - Get process ID
 */
static int32_t sys_getpid(uint32_t a1 UNUSED, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                          uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    process_t *proc = process_get_current();
    return proc ? proc->pid : 0;
}

/*
 * SYS_GETPPID - Get parent process ID
 */
static int32_t sys_getppid(uint32_t a1 UNUSED, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                           uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    process_t *proc = process_get_current();
    return proc ? proc->parent_pid : 0;
}

/*
 * SYS_BRK - Change data segment size
 */
static int32_t sys_brk(uint32_t addr, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                       uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    if (addr == 0) {
        return proc->brk;
    }
    
    if (addr < proc->heap_start) {
        return -1;
    }
    
    /* Allocate new pages if needed */
    if (addr > proc->heap_end) {
        uint32_t old_end = ALIGN_UP(proc->heap_end, PAGE_SIZE);
        uint32_t new_end = ALIGN_UP(addr, PAGE_SIZE);
        
        for (uint32_t page = old_end; page < new_end; page += PAGE_SIZE) {
            void *phys = pmm_alloc_page();
            if (!phys) return -1;
            vmm_map_page(proc->page_dir, page, (uint32_t)phys, 
                         PTE_USER | PTE_WRITABLE);
        }
        
        proc->heap_end = new_end;
    }
    
    proc->brk = addr;
    return addr;
}

/*
 * SYS_TIME - Get time
 */
static int32_t sys_time(uint32_t tloc, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                        uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    time_t t = timer_get_uptime();
    if (tloc) {
        *(time_t *)tloc = t;
    }
    return t;
}

/*
 * SYS_NANOSLEEP - Sleep
 */
static int32_t sys_nanosleep(uint32_t req, uint32_t rem UNUSED, uint32_t a3 UNUSED,
                             uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    struct timespec {
        time_t tv_sec;
        long tv_nsec;
    } *ts = (struct timespec *)req;
    
    uint32_t ms = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    thread_sleep(ms);
    
    return 0;
}

/*
 * SYS_GETCWD - Get current working directory
 */
static int32_t sys_getcwd(uint32_t buf, uint32_t size, uint32_t a3 UNUSED,
                          uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    process_t *proc = process_get_current();
    if (!proc) return 0;
    
    size_t len = strlen(proc->cwd);
    if (len >= size) return 0;
    
    strcpy((char *)buf, proc->cwd);
    return buf;
}

/*
 * SYS_CHDIR - Change directory
 */
static int32_t sys_chdir(uint32_t path, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                         uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    /* TODO: Verify path exists */
    strncpy(proc->cwd, (const char *)path, sizeof(proc->cwd) - 1);
    return 0;
}

/*
 * SYS_MMAP - Memory map
 */
static int32_t sys_mmap(uint32_t addr, uint32_t length, uint32_t prot,
                        uint32_t flags, uint32_t fd UNUSED, uint32_t offset UNUSED) {
    UNUSED uint32_t p = prot;
    UNUSED uint32_t f = flags;
    
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    /* Simple anonymous mapping */
    if (addr == 0) {
        addr = proc->heap_end;
    }
    
    uint32_t pages = ALIGN_UP(length, PAGE_SIZE) / PAGE_SIZE;
    
    for (uint32_t i = 0; i < pages; i++) {
        void *phys = pmm_alloc_page();
        if (!phys) return -1;
        vmm_map_page(proc->page_dir, addr + i * PAGE_SIZE, 
                     (uint32_t)phys, PTE_USER | PTE_WRITABLE);
    }
    
    if (addr + pages * PAGE_SIZE > proc->heap_end) {
        proc->heap_end = addr + pages * PAGE_SIZE;
    }
    
    return addr;
}

/*
 * SYS_MUNMAP - Unmap memory
 */
static int32_t sys_munmap(uint32_t addr, uint32_t length, uint32_t a3 UNUSED,
                          uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    uint32_t pages = ALIGN_UP(length, PAGE_SIZE) / PAGE_SIZE;
    
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t phys = vmm_get_physical(proc->page_dir, addr + i * PAGE_SIZE);
        if (phys) {
            vmm_unmap_page(proc->page_dir, addr + i * PAGE_SIZE);
            pmm_free_page((void *)phys);
        }
    }
    
    return 0;
}

/*
 * SYS_UNAME - Get system name
 */
static int32_t sys_uname(uint32_t buf, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                         uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    struct utsname {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
        char domainname[65];
    } *u = (struct utsname *)buf;
    
    strcpy(u->sysname, "AtomOS");
    strcpy(u->nodename, "atom");
    strcpy(u->release, ATOMOS_VERSION_STRING);
    strcpy(u->version, "#1 AtomOS " __DATE__);
    strcpy(u->machine, "i686");
    strcpy(u->domainname, "(none)");
    
    return 0;
}

/*
 * AtomOS debug syscall
 */
static int32_t sys_atomos_debug(uint32_t msg, uint32_t a2 UNUSED, uint32_t a3 UNUSED,
                                uint32_t a4 UNUSED, uint32_t a5 UNUSED, uint32_t a6 UNUSED) {
    kprintf("[USERSPACE] %s\n", (const char *)msg);
    return 0;
}

/*
 * Register a syscall handler
 */
void syscall_register(uint32_t num, syscall_handler_t handler) {
    if (num < MAX_SYSCALLS) {
        syscall_table[num] = handler;
    }
}

/*
 * Syscall dispatcher
 */
void syscall_handler(registers_t *regs) {
    uint32_t num = regs->eax;
    
    if (num >= MAX_SYSCALLS || syscall_table[num] == NULL) {
        kdebug("Unknown syscall: %d\n", num);
        regs->eax = -1;
        return;
    }
    
    /* Get arguments from registers (Linux x86 calling convention) */
    uint32_t a1 = regs->ebx;
    uint32_t a2 = regs->ecx;
    uint32_t a3 = regs->edx;
    uint32_t a4 = regs->esi;
    uint32_t a5 = regs->edi;
    uint32_t a6 = regs->ebp;
    
    /* Call handler and store result in EAX */
    regs->eax = syscall_table[num](a1, a2, a3, a4, a5, a6);
}

/*
 * Initialize syscall interface
 */
void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));
    
    /* Register Linux-compatible syscalls */
    syscall_register(SYS_EXIT, sys_exit);
    syscall_register(SYS_FORK, sys_fork);
    syscall_register(SYS_READ, sys_read);
    syscall_register(SYS_WRITE, sys_write);
    syscall_register(SYS_OPEN, sys_open);
    syscall_register(SYS_CLOSE, sys_close);
    syscall_register(SYS_WAITPID, sys_waitpid);
    syscall_register(SYS_EXECVE, sys_execve);
    syscall_register(SYS_GETPID, sys_getpid);
    syscall_register(SYS_GETPPID, sys_getppid);
    syscall_register(SYS_BRK, sys_brk);
    syscall_register(SYS_TIME, sys_time);
    syscall_register(SYS_NANOSLEEP, sys_nanosleep);
    syscall_register(SYS_GETCWD, sys_getcwd);
    syscall_register(SYS_CHDIR, sys_chdir);
    syscall_register(SYS_MMAP, sys_mmap);
    syscall_register(SYS_MUNMAP, sys_munmap);
    syscall_register(SYS_UNAME, sys_uname);
    
    /* AtomOS specific */
    syscall_register(SYS_ATOMOS_DEBUG, sys_atomos_debug);
    
    /* Register interrupt handler for INT 0x80 */
    register_interrupt_handler(SYSCALL_INT, (isr_handler_t)syscall_handler);
    
    kprintf("Syscall interface initialized\n");
}
