/*
 * AtomOS - Process Management Implementation
 */

#include "process.h"
#include "../include/kernel.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../arch/x86/gdt.h"
#include "../drivers/timer.h"

/* Process table */
static process_t *process_table[MAX_PROCESSES];
static pid_t next_pid = 1;
static tid_t next_tid = 1;

/* Scheduler state */
static scheduler_t scheduler;
static process_t *current_process = NULL;
static thread_t *current_thread = NULL;

/* Idle thread */
static thread_t *idle_thread;

/*
 * Idle thread function
 */
static void idle_thread_func(void) {
    while (1) {
        hlt();
    }
}

/*
 * Allocate a new PID
 */
static pid_t alloc_pid(void) {
    for (pid_t i = 0; i < MAX_PROCESSES; i++) {
        pid_t pid = (next_pid + i) % MAX_PROCESSES;
        if (pid == 0) pid = 1;
        if (process_table[pid] == NULL) {
            next_pid = pid + 1;
            return pid;
        }
    }
    return 0;
}

/*
 * Initialize process subsystem
 */
void process_init(void) {
    memset(process_table, 0, sizeof(process_table));
    scheduler_init();
    
    /* Create kernel process (PID 0) */
    process_t *kernel = process_create("kernel", PROC_FLAG_KERNEL);
    kernel->pid = 0;
    kernel->page_dir = vmm_get_kernel_directory();
    
    kprintf("Process subsystem initialized\n");
}

/*
 * Initialize scheduler
 */
void scheduler_init(void) {
    memset(&scheduler, 0, sizeof(scheduler));
    
    /* Create idle thread */
    process_t *kernel = process_table[0];
    if (kernel) {
        idle_thread = thread_create(kernel, idle_thread_func, NULL);
        idle_thread->priority = -1;  /* Lowest priority */
    }
}

/*
 * Create a new process
 */
process_t *process_create(const char *name, uint32_t flags) {
    process_t *proc = (process_t *)kcalloc(1, sizeof(process_t));
    if (!proc) {
        return NULL;
    }
    
    proc->pid = alloc_pid();
    if (proc->pid == 0 && flags & PROC_FLAG_KERNEL) {
        proc->pid = 0;  /* Allow kernel to be PID 0 */
    }
    
    strncpy(proc->name, name, sizeof(proc->name) - 1);
    proc->state = PROCESS_CREATED;
    proc->flags = flags;
    proc->parent_pid = current_process ? current_process->pid : 0;
    
    /* Create address space */
    if (flags & PROC_FLAG_USER) {
        proc->page_dir = vmm_create_address_space();
    } else {
        proc->page_dir = vmm_get_kernel_directory();
    }
    
    /* Initialize heap */
    proc->heap_start = USER_SPACE_START + MB(4);
    proc->heap_end = proc->heap_start;
    proc->brk = proc->heap_start;
    
    /* Initialize file table */
    proc->next_fd = 3;  /* 0, 1, 2 reserved for stdin/stdout/stderr */
    
    /* Set working directory */
    strcpy(proc->cwd, "/");
    
    proc->start_time = timer_get_ms();
    
    /* Add to process table */
    if (proc->pid < MAX_PROCESSES) {
        process_table[proc->pid] = proc;
    }
    
    kdebug("Created process %d: %s\n", proc->pid, proc->name);
    
    return proc;
}

/*
 * Destroy a process
 */
void process_destroy(process_t *proc) {
    if (!proc) return;
    
    /* Remove from process table */
    if (proc->pid < MAX_PROCESSES) {
        process_table[proc->pid] = NULL;
    }
    
    /* Destroy threads */
    thread_t *thread = proc->main_thread;
    while (thread) {
        thread_t *next = thread->next;
        thread_destroy(thread);
        thread = next;
    }
    
    /* Free address space */
    if (proc->flags & PROC_FLAG_USER) {
        vmm_destroy_address_space(proc->page_dir);
    }
    
    /* Close files */
    for (int i = 0; i < 256; i++) {
        if (proc->file_table[i]) {
            kfree(proc->file_table[i]);
        }
    }
    
    kfree(proc);
}

/*
 * Get current process
 */
process_t *process_get_current(void) {
    return current_process;
}

/*
 * Get process by PID
 */
process_t *process_get_by_pid(pid_t pid) {
    if (pid >= MAX_PROCESSES) return NULL;
    return process_table[pid];
}

/*
 * Create a new thread
 */
thread_t *thread_create(process_t *proc, void (*entry)(void), void *arg) {
    UNUSED void *a = arg;
    
    thread_t *thread = (thread_t *)kcalloc(1, sizeof(thread_t));
    if (!thread) {
        return NULL;
    }
    
    thread->tid = next_tid++;
    thread->pid = proc->pid;
    thread->state = PROCESS_READY;
    thread->priority = 0;
    thread->time_slice = 10;  /* 10 ticks */
    
    /* Allocate kernel stack */
    thread->kernel_stack = (uint32_t)pmm_alloc_pages(KERNEL_STACK_SIZE / PAGE_SIZE);
    thread->kernel_stack += KERNEL_STACK_SIZE;  /* Stack grows down */
    
    /* Allocate user stack for user processes */
    if (proc->flags & PROC_FLAG_USER) {
        void *user_stack = pmm_alloc_pages(USER_STACK_SIZE / PAGE_SIZE);
        thread->user_stack = USER_STACK_TOP;
        vmm_map_range(proc->page_dir, USER_STACK_TOP - USER_STACK_SIZE,
                      (uint32_t)user_stack, USER_STACK_SIZE,
                      PTE_USER | PTE_WRITABLE);
    } else {
        thread->user_stack = thread->kernel_stack;
    }
    
    /* Initialize context */
    memset(&thread->context, 0, sizeof(context_t));
    thread->context.eip = (uint32_t)entry;
    thread->context.esp = thread->kernel_stack;
    thread->context.eflags = 0x202;  /* IF set */
    
    if (proc->flags & PROC_FLAG_USER) {
        thread->context.cs = GDT_USER_CODE | 0x3;
        thread->context.ss3 = GDT_USER_DATA | 0x3;
        thread->context.esp3 = thread->user_stack;
    } else {
        thread->context.cs = GDT_KERNEL_CODE;
    }
    
    /* Add to process */
    thread->next = proc->main_thread;
    proc->main_thread = thread;
    proc->thread_count++;
    
    /* Add to scheduler */
    scheduler_add(thread);
    
    return thread;
}

/*
 * Destroy a thread
 */
void thread_destroy(thread_t *thread) {
    if (!thread) return;
    
    scheduler_remove(thread);
    
    /* Free stacks */
    if (thread->kernel_stack) {
        pmm_free_pages((void *)(thread->kernel_stack - KERNEL_STACK_SIZE),
                       KERNEL_STACK_SIZE / PAGE_SIZE);
    }
    
    kfree(thread);
}

/*
 * Add thread to scheduler
 */
void scheduler_add(thread_t *thread) {
    cli();
    
    thread->next = scheduler.ready_queue;
    scheduler.ready_queue = thread;
    scheduler.total_threads++;
    
    sti();
}

/*
 * Remove thread from scheduler
 */
void scheduler_remove(thread_t *thread) {
    cli();
    
    thread_t *prev = NULL;
    thread_t *curr = scheduler.ready_queue;
    
    while (curr) {
        if (curr == thread) {
            if (prev) {
                prev->next = curr->next;
            } else {
                scheduler.ready_queue = curr->next;
            }
            scheduler.total_threads--;
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    
    sti();
}

/*
 * Context switch implementation (assembly helper needed)
 */
extern void switch_context(context_t *old, context_t *new);

/*
 * Schedule next thread
 */
void schedule(void) {
    if (!scheduler.ready_queue) {
        return;
    }
    
    cli();
    
    thread_t *old = current_thread;
    thread_t *new = scheduler.ready_queue;
    
    /* Find next ready thread */
    while (new && new->state != PROCESS_READY) {
        new = new->next;
    }
    
    if (!new) {
        new = idle_thread;
    }
    
    if (new == old) {
        sti();
        return;
    }
    
    /* Update states */
    if (old && old->state == PROCESS_RUNNING) {
        old->state = PROCESS_READY;
    }
    new->state = PROCESS_RUNNING;
    
    current_thread = new;
    current_process = process_get_by_pid(new->pid);
    
    /* Update TSS kernel stack */
    tss_set_kernel_stack(new->kernel_stack);
    
    /* Switch address space if needed */
    if (current_process && current_process->page_dir != vmm_get_current_directory()) {
        vmm_switch_address_space(current_process->page_dir);
    }
    
    scheduler.context_switches++;
    
    sti();
    
    /* Perform context switch */
    if (old) {
        switch_context(&old->context, &new->context);
    } else {
        switch_context(NULL, &new->context);
    }
}

/*
 * Yield current thread
 */
void thread_yield(void) {
    schedule();
}

/*
 * Exit current thread
 */
void thread_exit(int code) {
    if (!current_thread) return;
    
    current_thread->state = PROCESS_TERMINATED;
    
    process_t *proc = current_process;
    if (proc) {
        proc->exit_code = code;
        proc->thread_count--;
        
        if (proc->thread_count == 0) {
            proc->state = PROCESS_ZOMBIE;
        }
    }
    
    schedule();
    
    /* Should never return */
    while (1) hlt();
}

/*
 * Sleep for milliseconds
 */
void thread_sleep(uint32_t ms) {
    if (!current_thread) return;
    
    current_thread->state = PROCESS_BLOCKED;
    uint64_t wake_time = timer_get_ms() + ms;
    
    while (timer_get_ms() < wake_time) {
        schedule();
    }
    
    current_thread->state = PROCESS_READY;
}

/*
 * Exit current process
 */
void process_exit(int status) {
    if (!current_process) return;
    
    current_process->exit_code = status;
    current_process->state = PROCESS_ZOMBIE;
    
    /* Terminate all threads */
    thread_t *thread = current_process->main_thread;
    while (thread) {
        thread->state = PROCESS_TERMINATED;
        thread = thread->next;
    }
    
    schedule();
    while (1) hlt();
}

/*
 * Wait for child process
 */
pid_t process_wait(int *status) {
    return process_waitpid(-1, status, 0);
}

/*
 * Wait for specific child process
 */
pid_t process_waitpid(pid_t pid, int *status, int options) {
    (void)options;  /* Unused parameter */
    
    while (1) {
        for (int i = 1; i < MAX_PROCESSES; i++) {
            process_t *child = process_table[i];
            if (!child) continue;
            
            if (child->parent_pid != current_process->pid) continue;
            
            if (pid != (pid_t)-1 && child->pid != pid) continue;
            
            if (child->state == PROCESS_ZOMBIE) {
                if (status) {
                    *status = child->exit_code;
                }
                pid_t ret = child->pid;
                process_destroy(child);
                return ret;
            }
        }
        
        /* Wait for child to exit */
        current_thread->state = PROCESS_WAITING;
        schedule();
    }
}

/*
 * Execute a new program
 */
pid_t process_exec(const char *path, char *const argv[], char *const envp[]) {
    (void)path;
    (void)argv;
    (void)envp;
    /* TODO: Implement proper program loading */
    kerror("process_exec not fully implemented yet\n");
    return -1;
}

/*
 * Send signal to process
 */
void process_signal(pid_t pid, int sig) {
    (void)pid;
    (void)sig;
    /* TODO: Implement signal handling */
}

/*
 * Fork current process
 */
pid_t process_fork(void) {
    process_t *parent = current_process;
    process_t *child = process_create(parent->name, parent->flags);
    
    if (!child) {
        return -1;
    }
    
    /* Clone address space */
    if (parent->flags & PROC_FLAG_USER) {
        vmm_destroy_address_space(child->page_dir);
        child->page_dir = vmm_clone_address_space(parent->page_dir);
    }
    
    /* Copy memory layout */
    child->heap_start = parent->heap_start;
    child->heap_end = parent->heap_end;
    child->brk = parent->brk;
    
    /* Create main thread with same context */
    thread_t *child_thread = thread_create(child, NULL, NULL);
    memcpy(&child_thread->context, &current_thread->context, sizeof(context_t));
    
    /* Child returns 0, parent returns child PID */
    child_thread->context.eax = 0;
    return child->pid;
}
