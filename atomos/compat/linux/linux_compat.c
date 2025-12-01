/*
 * AtomOS - Linux Compatibility Layer Implementation
 * ELF loader and Linux syscall emulation
 */

#include "linux_compat.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/pmm.h"
#include "../../kernel/mm/vmm.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/fs/vfs.h"
#include "../../kernel/process/process.h"
#include "../../kernel/drivers/timer.h"

/*
 * Load an ELF executable
 */
int linux_load_elf(const char *path, uint32_t *entry_point) {
    /* Open file */
    fd_t fd = vfs_open(path, O_RDONLY, 0);
    if (fd < 0) {
        kerror("Failed to open ELF file: %s\n", path);
        return -1;
    }
    
    /* Read ELF header */
    elf32_header_t header;
    if (vfs_read(fd, &header, sizeof(header)) != sizeof(header)) {
        kerror("Failed to read ELF header\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Verify ELF magic */
    if (header.magic != ELF_MAGIC) {
        kerror("Invalid ELF magic: 0x%x\n", header.magic);
        vfs_close(fd);
        return -1;
    }
    
    /* Check if 32-bit */
    if (header.class != 1) {
        kerror("Only 32-bit ELF supported\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Check if x86 */
    if (header.machine != 3) {
        kerror("Only x86 ELF supported\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Get current process */
    process_t *proc = process_get_current();
    if (!proc) {
        vfs_close(fd);
        return -1;
    }
    
    /* Load program headers */
    for (int i = 0; i < header.phnum; i++) {
        elf32_phdr_t phdr;
        
        vfs_seek(fd, header.phoff + i * header.phentsize, SEEK_SET);
        if (vfs_read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
            kerror("Failed to read program header %d\n", i);
            continue;
        }
        
        /* Only load PT_LOAD segments */
        if (phdr.type != PT_LOAD) {
            continue;
        }
        
        /* Calculate pages needed */
        uint32_t start = ALIGN_DOWN(phdr.vaddr, PAGE_SIZE);
        uint32_t end = ALIGN_UP(phdr.vaddr + phdr.memsz, PAGE_SIZE);
        uint32_t pages = (end - start) / PAGE_SIZE;
        
        /* Map pages */
        uint32_t flags = PTE_USER;
        if (phdr.flags & PF_W) flags |= PTE_WRITABLE;
        
        for (uint32_t page = 0; page < pages; page++) {
            uint32_t vaddr = start + page * PAGE_SIZE;
            
            if (!vmm_is_mapped(proc->page_dir, vaddr)) {
                void *phys = pmm_alloc_page();
                if (!phys) {
                    kerror("Out of memory loading ELF\n");
                    vfs_close(fd);
                    return -1;
                }
                vmm_map_page(proc->page_dir, vaddr, (uint32_t)phys, flags);
            }
        }
        
        /* Read segment data */
        if (phdr.filesz > 0) {
            vfs_seek(fd, phdr.offset, SEEK_SET);
            vfs_read(fd, (void *)phdr.vaddr, phdr.filesz);
        }
        
        /* Zero remaining memory */
        if (phdr.memsz > phdr.filesz) {
            memset((void *)(phdr.vaddr + phdr.filesz), 0, 
                   phdr.memsz - phdr.filesz);
        }
    }
    
    vfs_close(fd);
    
    *entry_point = header.entry;
    return 0;
}

/*
 * Execute a Linux ELF binary
 */
int linux_exec(const char *path, char *const argv[], char *const envp[]) {
    uint32_t entry;
    
    /* Load ELF */
    if (linux_load_elf(path, &entry) < 0) {
        return -1;
    }
    
    /* Setup stack with arguments */
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    /* Count arguments and environment */
    int argc = 0, envc = 0;
    if (argv) while (argv[argc]) argc++;
    if (envp) while (envp[envc]) envc++;
    
    /* Allocate stack space for arguments */
    uint32_t *stack = (uint32_t *)USER_STACK_TOP;
    stack--;
    
    /* Push environment strings and save pointers */
    char **env_ptrs = (char **)kmalloc((envc + 1) * sizeof(char *));
    for (int i = envc - 1; i >= 0; i--) {
        size_t len = strlen(envp[i]) + 1;
        stack = (uint32_t *)((uint32_t)stack - len);
        memcpy(stack, envp[i], len);
        env_ptrs[i] = (char *)stack;
    }
    env_ptrs[envc] = NULL;
    
    /* Push argument strings and save pointers */
    char **arg_ptrs = (char **)kmalloc((argc + 1) * sizeof(char *));
    for (int i = argc - 1; i >= 0; i--) {
        size_t len = strlen(argv[i]) + 1;
        stack = (uint32_t *)((uint32_t)stack - len);
        memcpy(stack, argv[i], len);
        arg_ptrs[i] = (char *)stack;
    }
    arg_ptrs[argc] = NULL;
    
    /* Align stack */
    stack = (uint32_t *)ALIGN_DOWN((uint32_t)stack, 16);
    
    /* Push NULL auxiliary vector terminator */
    *--stack = 0;
    *--stack = 0;
    
    /* Push environment pointers */
    *--stack = 0;  /* NULL terminator */
    for (int i = envc - 1; i >= 0; i--) {
        *--stack = (uint32_t)env_ptrs[i];
    }
    
    /* Push argument pointers */
    *--stack = 0;  /* NULL terminator */
    for (int i = argc - 1; i >= 0; i--) {
        *--stack = (uint32_t)arg_ptrs[i];
    }
    
    /* Push argc */
    *--stack = argc;
    
    kfree(arg_ptrs);
    kfree(env_ptrs);
    
    /* Mark process as Linux binary */
    proc->flags |= PROC_FLAG_LINUX;
    
    /* Enter user mode */
    enter_usermode(entry, (uint32_t)stack);
    
    /* Should never reach here */
    return 0;
}

/*
 * Linux syscall implementations
 */

int linux_sys_read(int fd, void *buf, size_t count) {
    return vfs_read(fd, buf, count);
}

int linux_sys_write(int fd, const void *buf, size_t count) {
    /* Handle stdout/stderr specially for debugging */
    if (fd == 1 || fd == 2) {
        const char *str = (const char *)buf;
        for (size_t i = 0; i < count; i++) {
            kprintf("%c", str[i]);
        }
        return count;
    }
    return vfs_write(fd, buf, count);
}

int linux_sys_open(const char *path, int flags, int mode) {
    return vfs_open(path, flags, mode);
}

int linux_sys_close(int fd) {
    return vfs_close(fd);
}

int linux_sys_stat(const char *path, void *buf) {
    return vfs_stat(path, (stat_t *)buf);
}

int linux_sys_fstat(int fd, void *buf) {
    return vfs_fstat(fd, (stat_t *)buf);
}

int linux_sys_lseek(int fd, off_t offset, int whence) {
    return vfs_seek(fd, offset, whence);
}

int linux_sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    UNUSED int p = prot;
    UNUSED int f = flags;
    UNUSED int d = fd;
    UNUSED off_t o = off;
    
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    uint32_t vaddr = (uint32_t)addr;
    if (vaddr == 0) {
        vaddr = proc->heap_end;
    }
    
    uint32_t pages = ALIGN_UP(len, PAGE_SIZE) / PAGE_SIZE;
    
    for (uint32_t i = 0; i < pages; i++) {
        void *phys = pmm_alloc_page();
        if (!phys) return -1;
        vmm_map_page(proc->page_dir, vaddr + i * PAGE_SIZE, 
                     (uint32_t)phys, PTE_USER | PTE_WRITABLE);
    }
    
    if (vaddr + pages * PAGE_SIZE > proc->heap_end) {
        proc->heap_end = vaddr + pages * PAGE_SIZE;
    }
    
    return vaddr;
}

int linux_sys_munmap(void *addr, size_t len) {
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    uint32_t pages = ALIGN_UP(len, PAGE_SIZE) / PAGE_SIZE;
    uint32_t vaddr = (uint32_t)addr;
    
    for (uint32_t i = 0; i < pages; i++) {
        vmm_unmap_page(proc->page_dir, vaddr + i * PAGE_SIZE);
    }
    
    return 0;
}

int linux_sys_brk(void *addr) {
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    if (addr == NULL) {
        return proc->brk;
    }
    
    uint32_t new_brk = (uint32_t)addr;
    
    if (new_brk < proc->heap_start) {
        return -1;
    }
    
    if (new_brk > proc->heap_end) {
        uint32_t old_end = ALIGN_UP(proc->heap_end, PAGE_SIZE);
        uint32_t new_end = ALIGN_UP(new_brk, PAGE_SIZE);
        
        for (uint32_t page = old_end; page < new_end; page += PAGE_SIZE) {
            void *phys = pmm_alloc_page();
            if (!phys) return -1;
            vmm_map_page(proc->page_dir, page, (uint32_t)phys, 
                         PTE_USER | PTE_WRITABLE);
        }
        
        proc->heap_end = new_end;
    }
    
    proc->brk = new_brk;
    return new_brk;
}

int linux_sys_ioctl(int fd, unsigned long request, void *arg) {
    UNUSED int f = fd;
    UNUSED unsigned long r = request;
    UNUSED void *a = arg;
    /* Stub - would need device-specific handling */
    return 0;
}

int linux_sys_getpid(void) {
    process_t *proc = process_get_current();
    return proc ? proc->pid : 0;
}

int linux_sys_fork(void) {
    return process_fork();
}

int linux_sys_execve(const char *path, char *const argv[], char *const envp[]) {
    return linux_exec(path, argv, envp);
}

int linux_sys_exit(int status) {
    process_exit(status);
    return 0;  /* Never reached */
}

int linux_sys_waitpid(int pid, int *status, int options) {
    return process_waitpid(pid, status, options);
}

int linux_sys_kill(int pid, int sig) {
    process_signal(pid, sig);
    return 0;
}

int linux_sys_uname(void *buf) {
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
    strcpy(u->release, "1.0.0");
    strcpy(u->version, "#1 AtomOS " __DATE__);
    strcpy(u->machine, "i686");
    strcpy(u->domainname, "(none)");
    
    return 0;
}

int linux_sys_getcwd(char *buf, size_t size) {
    process_t *proc = process_get_current();
    if (!proc) return 0;
    
    size_t len = strlen(proc->cwd);
    if (len >= size) return 0;
    
    strcpy(buf, proc->cwd);
    return (int)buf;
}

int linux_sys_chdir(const char *path) {
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    strncpy(proc->cwd, path, sizeof(proc->cwd) - 1);
    return 0;
}

int linux_sys_gettimeofday(linux_timeval_t *tv, linux_timezone_t *tz) {
    if (tv) {
        tv->tv_sec = timer_get_uptime();
        tv->tv_usec = (timer_get_ms() % 1000) * 1000;
    }
    if (tz) {
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }
    return 0;
}

int linux_sys_nanosleep(const void *req, void *rem) {
    UNUSED void *r = rem;
    struct timespec {
        long tv_sec;
        long tv_nsec;
    } *ts = (struct timespec *)req;
    
    uint32_t ms = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    thread_sleep(ms);
    
    return 0;
}

/*
 * Initialize Linux compatibility layer
 */
void linux_compat_init(void) {
    kprintf("Linux compatibility layer initialized\n");
}
