/*
 * AtomOS - Process Management
 * Process creation, scheduling, and management
 */

#ifndef _ATOMOS_PROCESS_H
#define _ATOMOS_PROCESS_H

#include "../include/types.h"
#include "../mm/vmm.h"
#include "../arch/x86/idt.h"

/* Process limits */
#define MAX_PROCESSES       256
#define MAX_THREADS         1024
#define KERNEL_STACK_SIZE   KB(8)
#define USER_STACK_SIZE     MB(1)

/* Process states */
typedef enum {
    PROCESS_CREATED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_WAITING,
    PROCESS_ZOMBIE,
    PROCESS_TERMINATED
} process_state_t;

/* Process flags */
#define PROC_FLAG_KERNEL    0x01
#define PROC_FLAG_USER      0x02
#define PROC_FLAG_DAEMON    0x04
#define PROC_FLAG_LINUX     0x10    /* Linux binary */
#define PROC_FLAG_WINDOWS   0x20    /* Windows binary */

/* Thread context */
typedef struct {
    /* Saved registers */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp3;  /* User stack pointer (ring 3) */
    uint32_t ss3;   /* User stack segment (ring 3) */
} context_t;

/* Thread structure */
typedef struct thread {
    tid_t tid;
    pid_t pid;
    context_t context;
    uint32_t kernel_stack;
    uint32_t user_stack;
    process_state_t state;
    int priority;
    uint32_t time_slice;
    uint64_t cpu_time;
    struct thread *next;
} thread_t;

/* File descriptor */
typedef struct {
    int fd;
    void *data;
    int type;
    int flags;
    off_t offset;
} fd_entry_t;

/* Process structure */
typedef struct process {
    pid_t pid;
    pid_t parent_pid;
    char name[64];
    process_state_t state;
    uint32_t flags;
    
    /* Memory */
    page_directory_t *page_dir;
    uint32_t heap_start;
    uint32_t heap_end;
    uint32_t brk;
    
    /* Threads */
    thread_t *main_thread;
    int thread_count;
    
    /* Files */
    fd_entry_t *file_table[256];
    int next_fd;
    
    /* Working directory */
    char cwd[256];
    
    /* Exit status */
    int exit_code;
    
    /* Timing */
    uint64_t start_time;
    uint64_t cpu_time;
    
    /* Linked list */
    struct process *next;
    struct process *prev;
} process_t;

/* Scheduler */
typedef struct {
    thread_t *current;
    thread_t *ready_queue;
    uint32_t total_threads;
    uint64_t context_switches;
} scheduler_t;

/* Function declarations */
void process_init(void);

/* Process operations */
process_t *process_create(const char *name, uint32_t flags);
void process_destroy(process_t *proc);
process_t *process_get_current(void);
process_t *process_get_by_pid(pid_t pid);

/* Thread operations */
thread_t *thread_create(process_t *proc, void (*entry)(void), void *arg);
void thread_destroy(thread_t *thread);
void thread_yield(void);
void thread_exit(int code);
void thread_sleep(uint32_t ms);

/* Scheduling */
void scheduler_init(void);
void scheduler_add(thread_t *thread);
void scheduler_remove(thread_t *thread);
void schedule(void);

/* User mode */
void enter_usermode(uint32_t entry, uint32_t stack);

/* Process loading */
pid_t process_exec(const char *path, char *const argv[], char *const envp[]);
pid_t process_fork(void);

/* Wait and exit */
void process_exit(int status);
pid_t process_wait(int *status);
pid_t process_waitpid(pid_t pid, int *status, int options);

/* Signals (basic) */
typedef void (*signal_handler_t)(int sig);
void process_signal(pid_t pid, int sig);
void process_set_signal_handler(int sig, signal_handler_t handler);

#endif /* _ATOMOS_PROCESS_H */
