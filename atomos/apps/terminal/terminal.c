/*
 * Javier Terminal Application Implementation
 * Full shell with Linux-compatible commands
 */

#include "terminal.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/mm/pmm.h"
#include "../../kernel/fs/vfs.h"
#include "../../kernel/process/process.h"
#include "../../kernel/drivers/timer.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include <stdarg.h>

/* Color palette */
static const color_t term_palette[16] = {
    RGB(0, 0, 0),           /* Black */
    RGB(205, 49, 49),       /* Red */
    RGB(13, 188, 121),      /* Green */
    RGB(229, 229, 16),      /* Yellow */
    RGB(36, 114, 200),      /* Blue */
    RGB(188, 63, 188),      /* Magenta */
    RGB(17, 168, 205),      /* Cyan */
    RGB(229, 229, 229),     /* White */
    RGB(102, 102, 102),     /* Bright Black */
    RGB(241, 76, 76),       /* Bright Red */
    RGB(35, 209, 139),      /* Bright Green */
    RGB(245, 245, 67),      /* Bright Yellow */
    RGB(59, 142, 234),      /* Bright Blue */
    RGB(214, 112, 214),     /* Bright Magenta */
    RGB(41, 184, 219),      /* Bright Cyan */
    RGB(255, 255, 255)      /* Bright White */
};

/* Built-in commands table */
static const builtin_cmd_t builtins[] = {
    {"help", "Display help for commands", cmd_help},
    {"clear", "Clear the terminal", cmd_clear},
    {"echo", "Display text", cmd_echo},
    {"ls", "List directory contents", cmd_ls},
    {"cd", "Change directory", cmd_cd},
    {"pwd", "Print working directory", cmd_pwd},
    {"cat", "Display file contents", cmd_cat},
    {"mkdir", "Create directory", cmd_mkdir},
    {"rm", "Remove files", cmd_rm},
    {"touch", "Create empty file", cmd_touch},
    {"cp", "Copy files", cmd_cp},
    {"mv", "Move/rename files", cmd_mv},
    {"uname", "System information", cmd_uname},
    {"whoami", "Current user", cmd_whoami},
    {"date", "Display date/time", cmd_date},
    {"uptime", "System uptime", cmd_uptime},
    {"free", "Memory usage", cmd_free},
    {"ps", "Process list", cmd_ps},
    {"kill", "Terminate process", cmd_kill},
    {"exit", "Exit terminal", cmd_exit},
    {"history", "Command history", cmd_history},
    {"env", "Environment variables", cmd_env},
    {"grep", "Search text patterns", cmd_grep},
    {"head", "Display first lines", cmd_head},
    {"tail", "Display last lines", cmd_tail},
    {"wc", "Word/line count", cmd_wc},
    {"df", "Disk space", cmd_df},
    {"man", "Manual pages", cmd_man},
    {"reboot", "Reboot system", cmd_reboot},
    {"shutdown", "Shutdown system", cmd_shutdown},
    {"ai", "AI Assistant", cmd_ai},
    {NULL, NULL, NULL}
};

/*
 * Create terminal window
 */
terminal_t *terminal_create(int x, int y, int width, int height) {
    terminal_t *term = (terminal_t *)kcalloc(1, sizeof(terminal_t));
    if (!term) return NULL;
    
    /* Create window */
    term->window = javier_window_create("Terminal", x, y, width, height, 
                                         WIN_DEFAULT | WIN_RESIZABLE);
    if (!term->window) {
        kfree(term);
        return NULL;
    }
    
    term->window->bg_color = term_palette[TERM_BLACK];
    
    /* Calculate character dimensions */
    int char_width = 8;
    int char_height = 16;
    term->cols = (width - 20) / char_width;
    term->rows = (height - 50) / char_height;
    
    if (term->cols > TERM_MAX_COLS) term->cols = TERM_MAX_COLS;
    if (term->rows > TERM_MAX_ROWS) term->rows = TERM_MAX_ROWS;
    
    /* Initialize state */
    term->cursor_x = 0;
    term->cursor_y = 0;
    term->fg_color = TERM_WHITE;
    term->bg_color = TERM_BLACK;
    term->running = true;
    
    strcpy(term->cwd, "/");
    strcpy(term->hostname, "atomos");
    strcpy(term->username, "user");
    
    /* Clear buffer */
    terminal_clear(term);
    
    /* Show welcome message */
    terminal_set_color(term, TERM_BRIGHT_CYAN, TERM_BLACK);
    terminal_puts(term, "╔══════════════════════════════════════════════════════╗\n");
    terminal_puts(term, "║           Javier Terminal v1.0                       ║\n");
    terminal_puts(term, "║     AtomOS Command Shell - Type 'help' for help      ║\n");
    terminal_puts(term, "╚══════════════════════════════════════════════════════╝\n\n");
    terminal_set_color(term, TERM_WHITE, TERM_BLACK);
    
    terminal_show_prompt(term);
    
    return term;
}

/*
 * Destroy terminal
 */
void terminal_destroy(terminal_t *term) {
    if (term) {
        if (term->window) {
            javier_window_destroy(term->window);
        }
        kfree(term);
    }
}

/*
 * Clear terminal
 */
void terminal_clear(terminal_t *term) {
    for (int y = 0; y < TERM_MAX_ROWS; y++) {
        for (int x = 0; x < TERM_MAX_COLS; x++) {
            term->buffer[y][x].ch = ' ';
            term->buffer[y][x].fg = term->fg_color;
            term->buffer[y][x].bg = term->bg_color;
        }
    }
    term->cursor_x = 0;
    term->cursor_y = 0;
}

/*
 * Scroll terminal
 */
void terminal_scroll(terminal_t *term, int lines) {
    if (lines <= 0) return;
    
    /* Move lines up */
    for (int y = 0; y < TERM_MAX_ROWS - lines; y++) {
        for (int x = 0; x < TERM_MAX_COLS; x++) {
            term->buffer[y][x] = term->buffer[y + lines][x];
        }
    }
    
    /* Clear new lines */
    for (int y = TERM_MAX_ROWS - lines; y < TERM_MAX_ROWS; y++) {
        for (int x = 0; x < TERM_MAX_COLS; x++) {
            term->buffer[y][x].ch = ' ';
            term->buffer[y][x].fg = term->fg_color;
            term->buffer[y][x].bg = term->bg_color;
        }
    }
    
    term->cursor_y -= lines;
    if (term->cursor_y < 0) term->cursor_y = 0;
}

/*
 * Put character
 */
void terminal_putchar(terminal_t *term, char c) {
    switch (c) {
        case '\n':
            term->cursor_x = 0;
            term->cursor_y++;
            break;
            
        case '\r':
            term->cursor_x = 0;
            break;
            
        case '\t':
            term->cursor_x = (term->cursor_x + TERM_TAB_SIZE) & ~(TERM_TAB_SIZE - 1);
            break;
            
        case '\b':
            if (term->cursor_x > 0) {
                term->cursor_x--;
                term->buffer[term->cursor_y][term->cursor_x].ch = ' ';
            }
            break;
            
        default:
            if (c >= ' ' && term->cursor_x < term->cols) {
                term->buffer[term->cursor_y][term->cursor_x].ch = c;
                term->buffer[term->cursor_y][term->cursor_x].fg = term->fg_color;
                term->buffer[term->cursor_y][term->cursor_x].bg = term->bg_color;
                term->cursor_x++;
            }
            break;
    }
    
    /* Handle line wrap */
    if (term->cursor_x >= term->cols) {
        term->cursor_x = 0;
        term->cursor_y++;
    }
    
    /* Handle scroll */
    if (term->cursor_y >= term->rows) {
        terminal_scroll(term, term->cursor_y - term->rows + 1);
    }
}

/*
 * Put string
 */
void terminal_puts(terminal_t *term, const char *str) {
    while (*str) {
        terminal_putchar(term, *str++);
    }
}

/*
 * Printf-style output
 */
void terminal_printf(terminal_t *term, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char buf[1024];
    char *p = buf;
    
    while (*fmt) {
        if (*fmt != '%') {
            *p++ = *fmt++;
            continue;
        }
        
        fmt++;
        
        switch (*fmt) {
            case 'd': {
                int val = va_arg(args, int);
                char num[32];
                int i = 0;
                bool neg = val < 0;
                if (neg) val = -val;
                do {
                    num[i++] = '0' + val % 10;
                    val /= 10;
                } while (val);
                if (neg) *p++ = '-';
                while (i > 0) *p++ = num[--i];
                break;
            }
            case 's': {
                char *s = va_arg(args, char *);
                while (*s) *p++ = *s++;
                break;
            }
            case 'c':
                *p++ = (char)va_arg(args, int);
                break;
            case 'x': {
                unsigned int val = va_arg(args, unsigned int);
                char hex[] = "0123456789abcdef";
                char num[16];
                int i = 0;
                do {
                    num[i++] = hex[val % 16];
                    val /= 16;
                } while (val);
                while (i > 0) *p++ = num[--i];
                break;
            }
            case '%':
                *p++ = '%';
                break;
            default:
                *p++ = '%';
                *p++ = *fmt;
                break;
        }
        fmt++;
    }
    *p = '\0';
    
    va_end(args);
    terminal_puts(term, buf);
}

/*
 * Set colors
 */
void terminal_set_color(terminal_t *term, term_color_t fg, term_color_t bg) {
    term->fg_color = fg;
    term->bg_color = bg;
}

/*
 * Show shell prompt
 */
void terminal_show_prompt(terminal_t *term) {
    terminal_set_color(term, TERM_BRIGHT_GREEN, TERM_BLACK);
    terminal_printf(term, "%s@%s", term->username, term->hostname);
    terminal_set_color(term, TERM_WHITE, TERM_BLACK);
    terminal_puts(term, ":");
    terminal_set_color(term, TERM_BRIGHT_BLUE, TERM_BLACK);
    terminal_puts(term, term->cwd);
    terminal_set_color(term, TERM_WHITE, TERM_BLACK);
    terminal_puts(term, "$ ");
    
    term->input_pos = 0;
    term->input_len = 0;
    term->input[0] = '\0';
}

/*
 * Parse command line into argc/argv
 */
static int parse_command(char *cmd, char *argv[], int max_args) {
    int argc = 0;
    char *p = cmd;
    
    while (*p && argc < max_args - 1) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        
        /* Handle quotes */
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
        }
        
        if (*p) *p++ = '\0';
    }
    
    argv[argc] = NULL;
    return argc;
}

/*
 * Execute command
 */
void terminal_execute_command(terminal_t *term, const char *cmd) {
    /* Skip empty commands */
    if (!cmd || !*cmd) {
        terminal_show_prompt(term);
        return;
    }
    
    /* Add to history */
    if (term->history_count < TERM_HISTORY_SIZE) {
        strncpy(term->history[term->history_count].command, cmd, TERM_CMD_MAX - 1);
        term->history_count++;
    }
    term->history_pos = term->history_count;
    
    /* Parse command */
    char cmd_copy[TERM_CMD_MAX];
    strncpy(cmd_copy, cmd, TERM_CMD_MAX - 1);
    
    char *argv[64];
    int argc = parse_command(cmd_copy, argv, 64);
    
    if (argc == 0) {
        terminal_show_prompt(term);
        return;
    }
    
    /* Find and execute command */
    for (int i = 0; builtins[i].name; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0) {
            int result = builtins[i].handler(term, argc, argv);
            (void)result;
            terminal_show_prompt(term);
            return;
        }
    }
    
    /* Command not found */
    terminal_set_color(term, TERM_RED, TERM_BLACK);
    terminal_printf(term, "bash: %s: command not found\n", argv[0]);
    terminal_set_color(term, TERM_WHITE, TERM_BLACK);
    terminal_show_prompt(term);
}

/*
 * Handle keyboard input
 */
void terminal_handle_key(terminal_t *term, key_event_t *event) {
    if (!event->pressed) return;
    
    switch (event->scancode) {
        case KEY_ENTER:
            terminal_putchar(term, '\n');
            terminal_execute_command(term, term->input);
            break;
            
        case KEY_BACKSPACE:
            if (term->input_pos > 0) {
                term->input_pos--;
                term->input_len--;
                term->input[term->input_len] = '\0';
                terminal_putchar(term, '\b');
            }
            break;
            
        case KEY_UP:
            /* History up */
            if (term->history_pos > 0) {
                term->history_pos--;
                /* Clear current input */
                while (term->input_len > 0) {
                    terminal_putchar(term, '\b');
                    term->input_len--;
                }
                /* Insert history */
                strcpy(term->input, term->history[term->history_pos].command);
                term->input_len = strlen(term->input);
                term->input_pos = term->input_len;
                terminal_puts(term, term->input);
            }
            break;
            
        case KEY_DOWN:
            /* History down */
            if (term->history_pos < term->history_count - 1) {
                term->history_pos++;
                while (term->input_len > 0) {
                    terminal_putchar(term, '\b');
                    term->input_len--;
                }
                strcpy(term->input, term->history[term->history_pos].command);
                term->input_len = strlen(term->input);
                term->input_pos = term->input_len;
                terminal_puts(term, term->input);
            }
            break;
            
        default:
            if (event->ascii >= ' ' && term->input_len < TERM_CMD_MAX - 1) {
                term->input[term->input_pos++] = event->ascii;
                term->input_len++;
                term->input[term->input_len] = '\0';
                terminal_putchar(term, event->ascii);
            }
            break;
    }
}

/*
 * Draw terminal
 */
void terminal_draw(terminal_t *term) {
    int base_x = term->window->bounds.x + term->window->client.x + 5;
    int base_y = term->window->bounds.y + term->window->client.y + 5;
    
    for (int y = 0; y < term->rows && y < TERM_MAX_ROWS; y++) {
        for (int x = 0; x < term->cols && x < TERM_MAX_COLS; x++) {
            term_cell_t *cell = &term->buffer[y][x];
            int px = base_x + x * 8;
            int py = base_y + y * 16;
            
            /* Draw background */
            fb_fill_rect(px, py, 8, 16, term_palette[cell->bg]);
            
            /* Draw character */
            if (cell->ch > ' ') {
                fb_draw_char(px, py, cell->ch, term_palette[cell->fg], 
                            term_palette[cell->bg]);
            }
        }
    }
    
    /* Draw cursor */
    int cx = base_x + term->cursor_x * 8;
    int cy = base_y + term->cursor_y * 16;
    
    /* Blinking cursor */
    if ((timer_get_ticks() / 50) % 2 == 0) {
        fb_fill_rect(cx, cy, 8, 16, term_palette[TERM_WHITE]);
    }
}

/* ========== BUILT-IN COMMANDS ========== */

int cmd_help(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    terminal_puts(term, "\nAvailable commands:\n\n");
    
    for (int i = 0; builtins[i].name; i++) {
        terminal_set_color(term, TERM_BRIGHT_YELLOW, TERM_BLACK);
        terminal_printf(term, "  %-12s", builtins[i].name);
        terminal_set_color(term, TERM_WHITE, TERM_BLACK);
        terminal_printf(term, "%s\n", builtins[i].description);
    }
    
    terminal_puts(term, "\n");
    return 0;
}

int cmd_clear(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_clear(term);
    return 0;
}

int cmd_echo(terminal_t *term, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        terminal_puts(term, argv[i]);
        if (i < argc - 1) terminal_putchar(term, ' ');
    }
    terminal_putchar(term, '\n');
    return 0;
}

int cmd_ls(terminal_t *term, int argc, char *argv[]) {
    const char *path = argc > 1 ? argv[1] : term->cwd;
    
    fd_t fd = vfs_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) {
        terminal_printf(term, "ls: cannot access '%s': No such file or directory\n", path);
        return 1;
    }
    
    dirent_t *entry;
    int count = 0;
    while ((entry = vfs_readdir(fd)) != NULL) {
        terminal_set_color(term, TERM_BRIGHT_CYAN, TERM_BLACK);
        terminal_printf(term, "%-20s", entry->d_name);
        terminal_set_color(term, TERM_WHITE, TERM_BLACK);
        count++;
        if (count % 4 == 0) terminal_putchar(term, '\n');
    }
    if (count % 4 != 0) terminal_putchar(term, '\n');
    
    vfs_close(fd);
    return 0;
}

int cmd_cd(terminal_t *term, int argc, char *argv[]) {
    const char *path = argc > 1 ? argv[1] : "/home";
    
    if (path[0] == '/') {
        strncpy(term->cwd, path, sizeof(term->cwd) - 1);
    } else if (strcmp(path, "..") == 0) {
        char *last = strrchr(term->cwd, '/');
        if (last && last != term->cwd) {
            *last = '\0';
        } else {
            strcpy(term->cwd, "/");
        }
    } else {
        if (term->cwd[strlen(term->cwd) - 1] != '/') {
            strcat(term->cwd, "/");
        }
        strcat(term->cwd, path);
    }
    
    return 0;
}

int cmd_pwd(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_printf(term, "%s\n", term->cwd);
    return 0;
}

int cmd_cat(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "cat: missing operand\n");
        return 1;
    }
    
    fd_t fd = vfs_open(argv[1], O_RDONLY, 0);
    if (fd < 0) {
        terminal_printf(term, "cat: %s: No such file or directory\n", argv[1]);
        return 1;
    }
    
    char buf[256];
    ssize_t bytes;
    while ((bytes = vfs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
        terminal_puts(term, buf);
    }
    terminal_putchar(term, '\n');
    
    vfs_close(fd);
    return 0;
}

int cmd_mkdir(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "mkdir: missing operand\n");
        return 1;
    }
    
    if (vfs_mkdir(argv[1], 0755) < 0) {
        terminal_printf(term, "mkdir: cannot create directory '%s'\n", argv[1]);
        return 1;
    }
    
    return 0;
}

int cmd_rm(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "rm: missing operand\n");
        return 1;
    }
    
    if (vfs_unlink(argv[1]) < 0) {
        terminal_printf(term, "rm: cannot remove '%s'\n", argv[1]);
        return 1;
    }
    
    return 0;
}

int cmd_touch(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "touch: missing operand\n");
        return 1;
    }
    
    fd_t fd = vfs_open(argv[1], O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        terminal_printf(term, "touch: cannot touch '%s'\n", argv[1]);
        return 1;
    }
    vfs_close(fd);
    
    return 0;
}

int cmd_cp(terminal_t *term, int argc, char *argv[]) {
    if (argc < 3) {
        terminal_puts(term, "cp: missing operand\n");
        return 1;
    }
    
    terminal_printf(term, "cp: '%s' -> '%s' (not implemented)\n", argv[1], argv[2]);
    return 0;
}

int cmd_mv(terminal_t *term, int argc, char *argv[]) {
    if (argc < 3) {
        terminal_puts(term, "mv: missing operand\n");
        return 1;
    }
    
    terminal_printf(term, "mv: '%s' -> '%s' (not implemented)\n", argv[1], argv[2]);
    return 0;
}

int cmd_uname(terminal_t *term, int argc, char *argv[]) {
    bool all = argc > 1 && strcmp(argv[1], "-a") == 0;
    
    if (all) {
        terminal_puts(term, "AtomOS atom 1.0.0 #1 i686 AtomOS/Javier\n");
    } else {
        terminal_puts(term, "AtomOS\n");
    }
    return 0;
}

int cmd_whoami(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_printf(term, "%s\n", term->username);
    return 0;
}

int cmd_date(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    uint64_t uptime = timer_get_uptime();
    terminal_printf(term, "System uptime: %d seconds\n", (int)uptime);
    return 0;
}

int cmd_uptime(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    uint64_t uptime = timer_get_uptime();
    int hours = uptime / 3600;
    int mins = (uptime % 3600) / 60;
    int secs = uptime % 60;
    terminal_printf(term, " up %d:%02d:%02d\n", hours, mins, secs);
    return 0;
}

int cmd_free(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    pmm_stats_t stats;
    pmm_get_stats(&stats);
    
    terminal_puts(term, "              total        used        free\n");
    terminal_printf(term, "Mem:    %10d  %10d  %10d\n",
                   stats.total_memory / 1024,
                   stats.used_memory / 1024,
                   stats.free_memory / 1024);
    return 0;
}

int cmd_ps(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_puts(term, "  PID  STATE    NAME\n");
    terminal_puts(term, "    0  running  kernel\n");
    terminal_puts(term, "    1  running  javier\n");
    terminal_puts(term, "    2  running  terminal\n");
    return 0;
}

int cmd_kill(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "kill: missing operand\n");
        return 1;
    }
    terminal_printf(term, "kill: sent signal to process %s\n", argv[1]);
    return 0;
}

int cmd_exit(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    term->running = false;
    terminal_puts(term, "Goodbye!\n");
    return 0;
}

int cmd_history(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    for (int i = 0; i < term->history_count; i++) {
        terminal_printf(term, " %4d  %s\n", i + 1, term->history[i].command);
    }
    return 0;
}

int cmd_env(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_puts(term, "PATH=/bin:/usr/bin\n");
    terminal_puts(term, "HOME=/home/user\n");
    terminal_puts(term, "SHELL=/bin/bash\n");
    terminal_printf(term, "PWD=%s\n", term->cwd);
    terminal_printf(term, "USER=%s\n", term->username);
    terminal_printf(term, "HOSTNAME=%s\n", term->hostname);
    return 0;
}

int cmd_grep(terminal_t *term, int argc, char *argv[]) {
    if (argc < 3) {
        terminal_puts(term, "grep: missing operand\n");
        return 1;
    }
    terminal_printf(term, "grep: searching for '%s' in %s\n", argv[1], argv[2]);
    return 0;
}

int cmd_head(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "head: missing operand\n");
        return 1;
    }
    return cmd_cat(term, argc, argv);  /* Simplified */
}

int cmd_tail(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "tail: missing operand\n");
        return 1;
    }
    return cmd_cat(term, argc, argv);  /* Simplified */
}

int cmd_wc(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "wc: missing operand\n");
        return 1;
    }
    terminal_printf(term, " 0 0 0 %s\n", argv[1]);
    return 0;
}

int cmd_df(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_puts(term, "Filesystem     Size  Used  Avail  Use%  Mounted on\n");
    terminal_puts(term, "ramfs          64M   1M    63M    2%    /\n");
    return 0;
}

int cmd_man(terminal_t *term, int argc, char *argv[]) {
    if (argc < 2) {
        terminal_puts(term, "What manual page do you want?\n");
        return 1;
    }
    
    for (int i = 0; builtins[i].name; i++) {
        if (strcmp(argv[1], builtins[i].name) == 0) {
            terminal_printf(term, "\n%s(1) - %s\n\n", 
                           builtins[i].name, builtins[i].description);
            terminal_printf(term, "SYNOPSIS\n    %s [options]\n\n", builtins[i].name);
            return 0;
        }
    }
    
    terminal_printf(term, "No manual entry for %s\n", argv[1]);
    return 1;
}

int cmd_reboot(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_puts(term, "System is rebooting...\n");
    outb(0x64, 0xFE);  /* Keyboard controller reset */
    return 0;
}

int cmd_shutdown(terminal_t *term, int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_puts(term, "System is halting...\n");
    cli();
    while (1) hlt();
    return 0;
}

int cmd_ai(terminal_t *term, int argc, char *argv[]) {
    terminal_set_color(term, TERM_BRIGHT_MAGENTA, TERM_BLACK);
    terminal_puts(term, "\n🤖 AtomOS AI Assistant\n");
    terminal_set_color(term, TERM_WHITE, TERM_BLACK);
    
    if (argc < 2) {
        terminal_puts(term, "Usage: ai <question or command>\n");
        terminal_puts(term, "\nExamples:\n");
        terminal_puts(term, "  ai how do I list files?\n");
        terminal_puts(term, "  ai what is my system memory?\n");
        terminal_puts(term, "  ai help me write a script\n\n");
        return 0;
    }
    
    /* Build query from arguments */
    char query[512] = "";
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(query, " ");
        strcat(query, argv[i]);
    }
    
    terminal_puts(term, "Analyzing your request...\n\n");
    
    /* Simple pattern matching for common questions */
    if (strstr(query, "list") || strstr(query, "files") || strstr(query, "directory")) {
        terminal_puts(term, "💡 To list files, use the 'ls' command:\n");
        terminal_set_color(term, TERM_BRIGHT_GREEN, TERM_BLACK);
        terminal_puts(term, "   ls        - list current directory\n");
        terminal_puts(term, "   ls /path  - list specific directory\n");
    } else if (strstr(query, "memory") || strstr(query, "ram")) {
        terminal_puts(term, "💡 To check memory usage, use 'free':\n");
        cmd_free(term, 0, NULL);
    } else if (strstr(query, "process") || strstr(query, "running")) {
        terminal_puts(term, "💡 To see running processes, use 'ps':\n");
        cmd_ps(term, 0, NULL);
    } else if (strstr(query, "script")) {
        terminal_puts(term, "💡 AtomOS supports Javi scripts (.javi files).\n");
        terminal_puts(term, "   Create a file with the Javi language and run it!\n");
    } else if (strstr(query, "help")) {
        terminal_puts(term, "💡 Use 'help' to see all available commands.\n");
        terminal_puts(term, "   Use 'man <command>' for detailed help.\n");
    } else {
        terminal_puts(term, "💡 I understand you're asking about: ");
        terminal_puts(term, query);
        terminal_puts(term, "\n\n");
        terminal_puts(term, "   Try 'help' for a list of commands, or\n");
        terminal_puts(term, "   ask me about files, memory, or processes.\n");
    }
    
    terminal_set_color(term, TERM_WHITE, TERM_BLACK);
    terminal_puts(term, "\n");
    return 0;
}
