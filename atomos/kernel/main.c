/*
 * AtomOS Kernel - Main Entry Point
 * Initializes all kernel subsystems and starts Javier desktop
 */

#include "include/kernel.h"
#include "include/types.h"
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "drivers/video/vga.h"
#include "drivers/video/framebuffer.h"
#include "drivers/keyboard/keyboard.h"
#include "drivers/timer.h"
#include "fs/vfs.h"
#include "process/process.h"
#include "syscall/syscall.h"
#include "../javier/core/javier.h"

/* Global boot information */
boot_info_t *g_boot_info = NULL;

/* External filesystem initialization */
extern void ramfs_init(void);

/*
 * Kernel panic - halt the system
 */
void panic(const char *message) {
    cli();
    
    vga_set_color(VGA_COLOR(VGA_WHITE, VGA_RED));
    vga_clear();
    
    vga_puts("\n\n");
    vga_puts("  ╔═══════════════════════════════════════════════════════════╗\n");
    vga_puts("  ║                    KERNEL PANIC                           ║\n");
    vga_puts("  ╚═══════════════════════════════════════════════════════════╝\n");
    vga_puts("\n");
    vga_puts("  An unrecoverable error has occurred.\n\n");
    vga_puts("  Error: ");
    vga_puts(message);
    vga_puts("\n\n");
    vga_puts("  The system has been halted. Please restart your computer.\n");
    vga_puts("\n");
    vga_puts("  ───────────────────────────────────────────────────────────\n");
    vga_puts("  AtomOS v" ATOMOS_VERSION_STRING " | Javier Desktop Environment\n");
    
    while (1) {
        hlt();
    }
}

/*
 * Kernel logging
 */
void klog(log_level_t level, const char *fmt, ...) {
    const char *prefixes[] = {
        "[DEBUG] ",
        "[INFO]  ",
        "[WARN]  ",
        "[ERROR] ",
        "[CRIT]  "
    };
    
    uint8_t colors[] = {
        VGA_COLOR(VGA_DARK_GREY, VGA_BLACK),
        VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK),
        VGA_COLOR(VGA_LIGHT_BROWN, VGA_BLACK),
        VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK),
        VGA_COLOR(VGA_WHITE, VGA_RED)
    };
    
    /* Set color and print prefix */
    uint8_t old_color = VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK);
    vga_set_color(colors[level]);
    vga_puts(prefixes[level]);
    vga_set_color(old_color);
    
    /* Print formatted message using simple printf */
    vga_printf(fmt);
}

/*
 * Display boot splash
 */
static void show_boot_splash(void) {
    vga_set_color(VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
    vga_clear();
    
    vga_puts("\n");
    vga_puts("     █████╗ ████████╗ ██████╗ ███╗   ███╗ ██████╗ ███████╗\n");
    vga_puts("    ██╔══██╗╚══██╔══╝██╔═══██╗████╗ ████║██╔═══██╗██╔════╝\n");
    vga_puts("    ███████║   ██║   ██║   ██║██╔████╔██║██║   ██║███████╗\n");
    vga_puts("    ██╔══██║   ██║   ██║   ██║██║╚██╔╝██║██║   ██║╚════██║\n");
    vga_puts("    ██║  ██║   ██║   ╚██████╔╝██║ ╚═╝ ██║╚██████╔╝███████║\n");
    vga_puts("    ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝     ╚═╝ ╚═════╝ ╚══════╝\n");
    vga_puts("\n");
    vga_set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
    vga_puts("    Version " ATOMOS_VERSION_STRING " | Javier Desktop Environment\n");
    vga_puts("    ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
}

/*
 * Kernel main entry point
 * Called from Stage 2 bootloader after protected mode is enabled
 */
void kernel_main(boot_info_t *boot_info) {
    /* Save boot info */
    g_boot_info = boot_info;
    
    /* Initialize VGA text mode for early output */
    vga_init();
    show_boot_splash();
    
    kprintf("Starting AtomOS kernel...\n\n");
    
    /* Initialize Global Descriptor Table */
    kprintf("Initializing GDT...\n");
    gdt_init();
    
    /* Initialize Interrupt Descriptor Table */
    kprintf("Initializing IDT...\n");
    idt_init();
    
    /* Enable interrupts */
    kprintf("Enabling interrupts...\n");
    sti();
    
    /* Initialize Physical Memory Manager */
    kprintf("Initializing physical memory manager...\n");
    uint32_t memory_size = 64 * 1024 * 1024;  /* Assume 64MB for now */
    if (boot_info && boot_info->total_memory) {
        memory_size = boot_info->total_memory;
    }
    pmm_init(memory_size, NULL, 0);
    
    /* Initialize Virtual Memory Manager */
    kprintf("Initializing virtual memory manager...\n");
    vmm_init();
    
    /* Initialize kernel heap */
    kprintf("Initializing kernel heap...\n");
    heap_init();
    
    /* Initialize timer */
    kprintf("Initializing timer...\n");
    timer_init();
    
    /* Initialize keyboard */
    kprintf("Initializing keyboard...\n");
    keyboard_init();
    
    /* Initialize Virtual File System */
    kprintf("Initializing VFS...\n");
    vfs_init();
    
    /* Initialize RAM filesystem */
    kprintf("Initializing ramfs...\n");
    ramfs_init();
    
    /* Mount root filesystem */
    kprintf("Mounting root filesystem...\n");
    vfs_mount(NULL, "/", "ramfs", 0);
    
    /* Initialize system calls */
    kprintf("Initializing syscall interface...\n");
    syscall_init();
    
    /* Initialize process subsystem */
    kprintf("Initializing process subsystem...\n");
    process_init();
    
    kprintf("\n");
    kprintf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    kprintf("AtomOS kernel initialized successfully!\n");
    kprintf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    /* Check if we have framebuffer for graphics */
    if (boot_info && boot_info->framebuffer_addr && 
        boot_info->screen_width && boot_info->screen_height) {
        
        kprintf("Initializing Javier desktop environment...\n");
        kprintf("  Resolution: %dx%d, %d bpp\n", 
                boot_info->screen_width, 
                boot_info->screen_height,
                boot_info->bits_per_pixel);
        
        /* Initialize framebuffer */
        fb_init(boot_info->framebuffer_addr,
                boot_info->screen_width,
                boot_info->screen_height,
                boot_info->pitch,
                boot_info->bits_per_pixel);
        
        /* Initialize and run Javier */
        javier_init();
        
        /* Create welcome window */
        javier_window_t *welcome = javier_window_create(
            "Welcome to AtomOS", 100, 100, 400, 300, WIN_DEFAULT);
        
        javier_label_create(welcome, "Welcome to AtomOS!", 20, 20, 360, 20);
        javier_label_create(welcome, "A clean, modern operating system", 20, 50, 360, 20);
        javier_label_create(welcome, "built from scratch in pure C.", 20, 70, 360, 20);
        javier_button_create(welcome, "Get Started", 150, 200, 100, 32);
        
        /* Run desktop */
        javier_run();
    } else {
        /* Text mode shell fallback */
        kprintf("No framebuffer available, starting text shell...\n\n");
        kprintf("AtomOS Shell v1.0\n");
        kprintf("Type 'help' for available commands.\n\n");
        
        /* Simple command shell */
        char cmd[256];
        while (1) {
            vga_set_color(VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            vga_puts("atomos");
            vga_set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
            vga_puts("$ ");
            
            /* Read command */
            int i = 0;
            while (i < 255) {
                char c = keyboard_get_char();
                if (c == '\n') {
                    vga_putchar('\n');
                    break;
                } else if (c == '\b' && i > 0) {
                    i--;
                    vga_putchar('\b');
                } else if (c >= ' ') {
                    cmd[i++] = c;
                    vga_putchar(c);
                }
            }
            cmd[i] = '\0';
            
            /* Process command */
            if (strcmp(cmd, "help") == 0) {
                vga_puts("Available commands:\n");
                vga_puts("  help     - Show this help\n");
                vga_puts("  clear    - Clear screen\n");
                vga_puts("  version  - Show version\n");
                vga_puts("  memory   - Show memory info\n");
                vga_puts("  uptime   - Show system uptime\n");
                vga_puts("  reboot   - Restart system\n");
            } else if (strcmp(cmd, "clear") == 0) {
                vga_clear();
            } else if (strcmp(cmd, "version") == 0) {
                vga_puts(ATOMOS_NAME " v" ATOMOS_VERSION_STRING "\n");
                vga_puts("Javier Desktop Environment v" JAVIER_VERSION_STRING "\n");
            } else if (strcmp(cmd, "memory") == 0) {
                pmm_stats_t stats;
                pmm_get_stats(&stats);
                vga_printf("Total: %d KB\n", stats.total_memory / 1024);
                vga_printf("Used:  %d KB\n", stats.used_memory / 1024);
                vga_printf("Free:  %d KB\n", stats.free_memory / 1024);
            } else if (strcmp(cmd, "uptime") == 0) {
                uint64_t uptime = timer_get_uptime();
                vga_printf("Uptime: %d seconds\n", (int)uptime);
            } else if (strcmp(cmd, "reboot") == 0) {
                vga_puts("Rebooting...\n");
                outb(0x64, 0xFE);  /* Keyboard controller reset */
            } else if (cmd[0] != '\0') {
                vga_puts("Unknown command: ");
                vga_puts(cmd);
                vga_puts("\n");
            }
        }
    }
    
    /* Should never reach here */
    while (1) {
        hlt();
    }
}
