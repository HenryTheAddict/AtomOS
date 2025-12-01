/*
 * Javier Terminal Application
 * Full-featured terminal emulator with command shell
 */

#ifndef _JAVIER_TERMINAL_H
#define _JAVIER_TERMINAL_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"
#include "../../kernel/drivers/keyboard/keyboard.h"

/* Terminal configuration */
#define TERM_MAX_COLS       120
#define TERM_MAX_ROWS       50
#define TERM_HISTORY_SIZE   100
#define TERM_CMD_MAX        512
#define TERM_TAB_SIZE       4

/* Terminal colors (ANSI-like) */
typedef enum {
    TERM_BLACK = 0,
    TERM_RED,
    TERM_GREEN,
    TERM_YELLOW,
    TERM_BLUE,
    TERM_MAGENTA,
    TERM_CYAN,
    TERM_WHITE,
    TERM_BRIGHT_BLACK,
    TERM_BRIGHT_RED,
    TERM_BRIGHT_GREEN,
    TERM_BRIGHT_YELLOW,
    TERM_BRIGHT_BLUE,
    TERM_BRIGHT_MAGENTA,
    TERM_BRIGHT_CYAN,
    TERM_BRIGHT_WHITE
} term_color_t;

/* Terminal cell */
typedef struct {
    char ch;
    term_color_t fg;
    term_color_t bg;
    uint8_t attrs;  /* Bold, underline, etc. */
} term_cell_t;

/* Command history entry */
typedef struct {
    char command[TERM_CMD_MAX];
} history_entry_t;

/* Terminal state */
typedef struct {
    javier_window_t *window;
    term_cell_t buffer[TERM_MAX_ROWS][TERM_MAX_COLS];
    int cursor_x, cursor_y;
    int scroll_offset;
    int cols, rows;
    term_color_t fg_color;
    term_color_t bg_color;
    
    /* Current input line */
    char input[TERM_CMD_MAX];
    int input_pos;
    int input_len;
    
    /* Command history */
    history_entry_t history[TERM_HISTORY_SIZE];
    int history_count;
    int history_pos;
    
    /* Shell state */
    char cwd[256];
    char hostname[64];
    char username[32];
    bool running;
} terminal_t;

/* Built-in command handler */
typedef int (*cmd_handler_t)(terminal_t *term, int argc, char *argv[]);

/* Built-in command entry */
typedef struct {
    const char *name;
    const char *description;
    cmd_handler_t handler;
} builtin_cmd_t;

/* Terminal functions */
terminal_t *terminal_create(int x, int y, int width, int height);
void terminal_destroy(terminal_t *term);
void terminal_update(terminal_t *term);
void terminal_draw(terminal_t *term);

/* Output functions */
void terminal_putchar(terminal_t *term, char c);
void terminal_puts(terminal_t *term, const char *str);
void terminal_printf(terminal_t *term, const char *fmt, ...);
void terminal_set_color(terminal_t *term, term_color_t fg, term_color_t bg);
void terminal_clear(terminal_t *term);
void terminal_clear_line(terminal_t *term);
void terminal_scroll(terminal_t *term, int lines);

/* Cursor functions */
void terminal_set_cursor(terminal_t *term, int x, int y);
void terminal_cursor_home(terminal_t *term);
void terminal_cursor_newline(terminal_t *term);

/* Input handling */
void terminal_handle_key(terminal_t *term, key_event_t *event);
void terminal_execute_command(terminal_t *term, const char *cmd);

/* Shell prompt */
void terminal_show_prompt(terminal_t *term);

/* Built-in commands */
int cmd_help(terminal_t *term, int argc, char *argv[]);
int cmd_clear(terminal_t *term, int argc, char *argv[]);
int cmd_echo(terminal_t *term, int argc, char *argv[]);
int cmd_ls(terminal_t *term, int argc, char *argv[]);
int cmd_cd(terminal_t *term, int argc, char *argv[]);
int cmd_pwd(terminal_t *term, int argc, char *argv[]);
int cmd_cat(terminal_t *term, int argc, char *argv[]);
int cmd_mkdir(terminal_t *term, int argc, char *argv[]);
int cmd_rm(terminal_t *term, int argc, char *argv[]);
int cmd_touch(terminal_t *term, int argc, char *argv[]);
int cmd_cp(terminal_t *term, int argc, char *argv[]);
int cmd_mv(terminal_t *term, int argc, char *argv[]);
int cmd_uname(terminal_t *term, int argc, char *argv[]);
int cmd_whoami(terminal_t *term, int argc, char *argv[]);
int cmd_date(terminal_t *term, int argc, char *argv[]);
int cmd_uptime(terminal_t *term, int argc, char *argv[]);
int cmd_free(terminal_t *term, int argc, char *argv[]);
int cmd_ps(terminal_t *term, int argc, char *argv[]);
int cmd_kill(terminal_t *term, int argc, char *argv[]);
int cmd_exit(terminal_t *term, int argc, char *argv[]);
int cmd_history(terminal_t *term, int argc, char *argv[]);
int cmd_alias(terminal_t *term, int argc, char *argv[]);
int cmd_export(terminal_t *term, int argc, char *argv[]);
int cmd_env(terminal_t *term, int argc, char *argv[]);
int cmd_grep(terminal_t *term, int argc, char *argv[]);
int cmd_head(terminal_t *term, int argc, char *argv[]);
int cmd_tail(terminal_t *term, int argc, char *argv[]);
int cmd_wc(terminal_t *term, int argc, char *argv[]);
int cmd_sort(terminal_t *term, int argc, char *argv[]);
int cmd_uniq(terminal_t *term, int argc, char *argv[]);
int cmd_chmod(terminal_t *term, int argc, char *argv[]);
int cmd_chown(terminal_t *term, int argc, char *argv[]);
int cmd_df(terminal_t *term, int argc, char *argv[]);
int cmd_du(terminal_t *term, int argc, char *argv[]);
int cmd_find(terminal_t *term, int argc, char *argv[]);
int cmd_man(terminal_t *term, int argc, char *argv[]);
int cmd_reboot(terminal_t *term, int argc, char *argv[]);
int cmd_shutdown(terminal_t *term, int argc, char *argv[]);
int cmd_ai(terminal_t *term, int argc, char *argv[]);

#endif /* _JAVIER_TERMINAL_H */
