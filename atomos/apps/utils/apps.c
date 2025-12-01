/*
 * AtomOS Applications Implementation
 * Core utility applications
 */

#include "apps.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/timer.h"

/* Common colors */
#define APP_BG       RGB(30, 30, 35)
#define APP_PANEL    RGB(45, 45, 50)
#define APP_TEXT     RGB(220, 220, 220)
#define APP_ACCENT   RGB(100, 150, 255)
#define APP_BORDER   RGB(60, 60, 65)

/* Forward declarations for run functions */
static void run_text_editor(void);
static void run_calculator(void);
static void run_calendar(void);
static void run_clock(void);
static void run_notes(void);
static void run_todolist(void);
static void run_filemanager(void);
static void run_sysmonitor(void);
static void run_settings(void);
static void run_paint(void);
static void run_colorpicker(void);
static void run_timer(void);
static void run_stopwatch(void);
static void run_alarm(void);
static void run_converter(void);
static void run_weather(void);
static void run_dictionary(void);
static void run_scicalc(void);
static void run_contacts(void);
static void run_hexeditor(void);
static void run_codeeditor(void);
static void run_videoplayer(void);
static void run_recorder(void);
static void run_netmonitor(void);
static void run_fontviewer(void);
static void run_sysinfo(void);
static void run_about(void);
static void run_launcher(void);

/* App list */
app_info_t app_list[] = {
    {"Text Editor", "Simple text editor", "📝", run_text_editor},
    {"Calculator", "Basic calculator", "🔢", run_calculator},
    {"Calendar", "Date calendar", "📅", run_calendar},
    {"Clock", "Digital clock", "🕐", run_clock},
    {"Notes", "Quick notes", "📋", run_notes},
    {"Todo List", "Task manager", "✓", run_todolist},
    {"File Manager", "Browse files", "📁", run_filemanager},
    {"System Monitor", "Process viewer", "📊", run_sysmonitor},
    {"Settings", "System settings", "⚙", run_settings},
    {"Paint", "Drawing app", "🎨", run_paint},
    {"Color Picker", "Choose colors", "🎨", run_colorpicker},
    {"Timer", "Countdown timer", "⏱", run_timer},
    {"Stopwatch", "Time events", "⏱", run_stopwatch},
    {"Alarm", "Set alarms", "⏰", run_alarm},
    {"Converter", "Unit converter", "🔄", run_converter},
    {"Weather", "Weather info", "☀", run_weather},
    {"Dictionary", "Word lookup", "📖", run_dictionary},
    {"Sci Calculator", "Scientific calc", "🔬", run_scicalc},
    {"Contacts", "Address book", "👤", run_contacts},
    {"Hex Editor", "Binary editor", "🔧", run_hexeditor},
    {"Code Editor", "Source editor", "💻", run_codeeditor},
    {"Video Player", "Play videos", "🎬", run_videoplayer},
    {"Recorder", "Audio recorder", "🎤", run_recorder},
    {"Network", "Network status", "🌐", run_netmonitor},
    {"Font Viewer", "Preview fonts", "A", run_fontviewer},
    {"System Info", "OS information", "ℹ", run_sysinfo},
    {"About", "About AtomOS", "?", run_about},
    {"Launcher", "App launcher", "🚀", run_launcher},
};

int app_count = sizeof(app_list) / sizeof(app_list[0]);

/* ==================== TEXT EDITOR ==================== */
text_editor_t *text_editor_create(void) {
    text_editor_t *ed = (text_editor_t *)kcalloc(1, sizeof(text_editor_t));
    if (!ed) return NULL;
    
    ed->window = javier_window_create("Text Editor", 50, 50, 600, 400, WIN_DEFAULT);
    ed->buf_size = 65536;
    ed->buffer = (char *)kcalloc(1, ed->buf_size);
    ed->running = true;
    
    return ed;
}

void text_editor_destroy(text_editor_t *ed) {
    if (ed) {
        if (ed->buffer) kfree(ed->buffer);
        if (ed->window) javier_window_destroy(ed->window);
        kfree(ed);
    }
}

void text_editor_draw(text_editor_t *ed) {
    if (!ed || !ed->window) return;
    
    int x = ed->window->bounds.x + ed->window->client.x;
    int y = ed->window->bounds.y + ed->window->client.y;
    int w = ed->window->client.width;
    int h = ed->window->client.height;
    
    /* Background */
    fb_fill_rect(x, y, w, h, RGB(40, 44, 52));
    
    /* Status bar */
    fb_fill_rect(x, y + h - 20, w, 20, APP_PANEL);
    
    char status[128];
    snprintf(status, sizeof(status), "Line %d, Col %d  %s%s", 
             ed->cursor_line + 1, ed->cursor_col + 1,
             ed->filename[0] ? ed->filename : "Untitled",
             ed->modified ? " *" : "");
    fb_draw_string(x + 5, y + h - 16, status, APP_TEXT, 0);
    
    /* Draw text */
    int line = 0;
    int col = 0;
    int ty = y + 5 - ed->scroll_y * 16;
    
    for (size_t i = 0; i <= ed->buf_len && ty < y + h - 25; i++) {
        char c = (i < ed->buf_len) ? ed->buffer[i] : '\0';
        
        if (c == '\n' || c == '\0') {
            line++;
            col = 0;
            ty += 16;
        } else {
            if (ty >= y && col < 80) {
                char s[2] = {c, 0};
                fb_draw_string(x + 5 + col * 8, ty, s, APP_TEXT, 0);
            }
            col++;
        }
        
        /* Draw cursor */
        if (line == ed->cursor_line && col == ed->cursor_col) {
            fb_fill_rect(x + 5 + col * 8, ty, 2, 14, APP_ACCENT);
        }
    }
}

void text_editor_update(text_editor_t *ed) {
    if (!ed) return;
    
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            switch (event.scancode) {
                case KEY_ESC:
                    ed->running = false;
                    break;
                case KEY_UP:
                    if (ed->cursor_line > 0) ed->cursor_line--;
                    break;
                case KEY_DOWN:
                    ed->cursor_line++;
                    break;
                case KEY_LEFT:
                    if (ed->cursor_col > 0) ed->cursor_col--;
                    break;
                case KEY_RIGHT:
                    ed->cursor_col++;
                    break;
                case KEY_BACKSPACE:
                    text_editor_delete(ed);
                    break;
                case KEY_ENTER:
                    text_editor_insert(ed, '\n');
                    break;
                default:
                    if (event.ascii >= 32 && event.ascii < 127) {
                        text_editor_insert(ed, event.ascii);
                    }
                    break;
            }
        }
    }
    
    text_editor_draw(ed);
}

void text_editor_insert(text_editor_t *ed, char c) {
    if (ed->buf_len >= ed->buf_size - 1) return;
    
    /* Find position in buffer */
    size_t pos = 0;
    int line = 0, col = 0;
    while (pos < ed->buf_len && (line < ed->cursor_line || 
           (line == ed->cursor_line && col < ed->cursor_col))) {
        if (ed->buffer[pos] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
        pos++;
    }
    
    /* Insert character */
    memmove(ed->buffer + pos + 1, ed->buffer + pos, ed->buf_len - pos);
    ed->buffer[pos] = c;
    ed->buf_len++;
    
    if (c == '\n') {
        ed->cursor_line++;
        ed->cursor_col = 0;
    } else {
        ed->cursor_col++;
    }
    
    ed->modified = true;
}

void text_editor_delete(text_editor_t *ed) {
    if (ed->buf_len == 0) return;
    
    /* Find position in buffer */
    size_t pos = 0;
    int line = 0, col = 0;
    while (pos < ed->buf_len && (line < ed->cursor_line || 
           (line == ed->cursor_line && col < ed->cursor_col))) {
        if (ed->buffer[pos] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
        pos++;
    }
    
    if (pos > 0) {
        pos--;
        if (ed->buffer[pos] == '\n') {
            ed->cursor_line--;
            /* Find column at end of previous line */
            ed->cursor_col = 0;
            size_t p = pos;
            while (p > 0 && ed->buffer[p-1] != '\n') {
                p--;
                ed->cursor_col++;
            }
        } else {
            ed->cursor_col--;
        }
        
        memmove(ed->buffer + pos, ed->buffer + pos + 1, ed->buf_len - pos);
        ed->buf_len--;
        ed->modified = true;
    }
}

static void run_text_editor(void) {
    text_editor_t *ed = text_editor_create();
    if (!ed) return;
    while (ed->running) {
        text_editor_update(ed);
        timer_sleep_ms(16);
    }
    text_editor_destroy(ed);
}

/* ==================== CALCULATOR ==================== */
calculator_t *calculator_create(void) {
    calculator_t *calc = (calculator_t *)kcalloc(1, sizeof(calculator_t));
    if (!calc) return NULL;
    
    calc->window = javier_window_create("Calculator", 100, 100, 250, 320, WIN_DEFAULT);
    strcpy(calc->display, "0");
    calc->new_number = true;
    calc->running = true;
    
    return calc;
}

void calculator_destroy(calculator_t *calc) {
    if (calc) {
        if (calc->window) javier_window_destroy(calc->window);
        kfree(calc);
    }
}

void calculator_draw(calculator_t *calc) {
    if (!calc || !calc->window) return;
    
    int x = calc->window->bounds.x + calc->window->client.x;
    int y = calc->window->bounds.y + calc->window->client.y;
    int w = calc->window->client.width;
    
    /* Background */
    fb_fill_rect(x, y, w, calc->window->client.height, APP_BG);
    
    /* Display */
    fb_fill_rect(x + 10, y + 10, w - 20, 50, RGB(50, 50, 55));
    fb_draw_string(x + w - 20 - strlen(calc->display) * 8, y + 25, 
                  calc->display, APP_TEXT, 0);
    
    /* Buttons */
    const char *buttons[] = {
        "C", "(", ")", "/",
        "7", "8", "9", "*",
        "4", "5", "6", "-",
        "1", "2", "3", "+",
        "0", ".", "=", ""
    };
    
    int bw = (w - 50) / 4;
    int bh = 40;
    
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 4; c++) {
            int idx = r * 4 + c;
            if (buttons[idx][0] == '\0') continue;
            
            int bx = x + 10 + c * (bw + 5);
            int by = y + 70 + r * (bh + 5);
            
            color_t bg = RGB(60, 60, 65);
            if (buttons[idx][0] >= '0' && buttons[idx][0] <= '9') {
                bg = RGB(70, 70, 75);
            } else if (buttons[idx][0] == '=') {
                bg = APP_ACCENT;
            }
            
            fb_fill_rect(bx, by, bw, bh, bg);
            fb_draw_string(bx + bw/2 - 4, by + 12, buttons[idx], APP_TEXT, 0);
        }
    }
}

void calculator_press(calculator_t *calc, char key) {
    if (key >= '0' && key <= '9') {
        if (calc->new_number) {
            calc->display[0] = key;
            calc->display[1] = '\0';
            calc->new_number = false;
        } else if (strlen(calc->display) < 15) {
            int len = strlen(calc->display);
            calc->display[len] = key;
            calc->display[len + 1] = '\0';
        }
    } else if (key == '.') {
        if (calc->new_number) {
            strcpy(calc->display, "0.");
            calc->new_number = false;
        } else if (!strchr(calc->display, '.') && strlen(calc->display) < 15) {
            strcat(calc->display, ".");
        }
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        calc->value1 = atoi(calc->display);
        calc->op = key;
        calc->new_number = true;
    } else if (key == '=') {
        calculator_equals(calc);
    } else if (key == 'C' || key == 'c') {
        calculator_clear(calc);
    }
}

void calculator_clear(calculator_t *calc) {
    strcpy(calc->display, "0");
    calc->value1 = 0;
    calc->value2 = 0;
    calc->op = 0;
    calc->new_number = true;
}

void calculator_equals(calculator_t *calc) {
    calc->value2 = atoi(calc->display);
    double result = 0;
    
    switch (calc->op) {
        case '+': result = calc->value1 + calc->value2; break;
        case '-': result = calc->value1 - calc->value2; break;
        case '*': result = calc->value1 * calc->value2; break;
        case '/': 
            if (calc->value2 != 0) result = calc->value1 / calc->value2;
            else strcpy(calc->display, "Error");
            break;
    }
    
    if (calc->op != '/' || calc->value2 != 0) {
        snprintf(calc->display, sizeof(calc->display), "%d", (int)result);
    }
    
    calc->new_number = true;
    calc->op = 0;
}

void calculator_update(calculator_t *calc) {
    if (!calc) return;
    
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            if (event.scancode == KEY_ESC) {
                calc->running = false;
            } else if (event.ascii) {
                calculator_press(calc, event.ascii);
            }
        }
    }
    
    calculator_draw(calc);
}

static void run_calculator(void) {
    calculator_t *calc = calculator_create();
    if (!calc) return;
    while (calc->running) {
        calculator_update(calc);
        timer_sleep_ms(16);
    }
    calculator_destroy(calc);
}

/* ==================== CALENDAR ==================== */
static int days_in_month(int year, int month) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
        return 29;
    }
    return days[month - 1];
}

static int day_of_week(int year, int month, int day) {
    /* Zeller's formula simplified */
    if (month < 3) { month += 12; year--; }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13*(month+1))/5 + k + k/4 + j/4 - 2*j) % 7;
    return ((h + 6) % 7);  /* 0 = Sunday */
}

calendar_t *calendar_create(void) {
    calendar_t *cal = (calendar_t *)kcalloc(1, sizeof(calendar_t));
    if (!cal) return NULL;
    
    cal->window = javier_window_create("Calendar", 100, 100, 300, 280, WIN_DEFAULT);
    cal->year = 2024;
    cal->month = 1;
    cal->day = 1;
    cal->selected_day = 1;
    cal->running = true;
    
    return cal;
}

void calendar_destroy(calendar_t *cal) {
    if (cal) {
        if (cal->window) javier_window_destroy(cal->window);
        kfree(cal);
    }
}

void calendar_draw(calendar_t *cal) {
    if (!cal || !cal->window) return;
    
    int x = cal->window->bounds.x + cal->window->client.x;
    int y = cal->window->bounds.y + cal->window->client.y;
    int w = cal->window->client.width;
    int h = cal->window->client.height;
    
    /* Background */
    fb_fill_rect(x, y, w, h, APP_BG);
    
    /* Month/Year header */
    const char *months[] = {"January", "February", "March", "April", "May", "June",
                           "July", "August", "September", "October", "November", "December"};
    char header[64];
    snprintf(header, sizeof(header), "< %s %d >", months[cal->month - 1], cal->year);
    fb_draw_string(x + (w - strlen(header) * 8) / 2, y + 10, header, APP_TEXT, 0);
    
    /* Day headers */
    const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    int cw = w / 7;
    for (int i = 0; i < 7; i++) {
        fb_draw_string(x + i * cw + cw/2 - 8, y + 35, days[i], RGB(150, 150, 150), 0);
    }
    
    /* Days */
    int first = day_of_week(cal->year, cal->month, 1);
    int num_days = days_in_month(cal->year, cal->month);
    int ch = 25;
    
    for (int d = 1; d <= num_days; d++) {
        int pos = first + d - 1;
        int row = pos / 7;
        int col = pos % 7;
        int dx = x + col * cw + cw/2 - 8;
        int dy = y + 55 + row * ch;
        
        if (d == cal->selected_day) {
            fb_fill_rect(dx - 4, dy - 2, 24, 18, APP_ACCENT);
        }
        
        char ds[4];
        snprintf(ds, sizeof(ds), "%2d", d);
        fb_draw_string(dx, dy, ds, d == cal->selected_day ? RGB(0, 0, 0) : APP_TEXT, 0);
    }
}

void calendar_next_month(calendar_t *cal) {
    cal->month++;
    if (cal->month > 12) {
        cal->month = 1;
        cal->year++;
    }
    cal->selected_day = 1;
}

void calendar_prev_month(calendar_t *cal) {
    cal->month--;
    if (cal->month < 1) {
        cal->month = 12;
        cal->year--;
    }
    cal->selected_day = 1;
}

void calendar_update(calendar_t *cal) {
    if (!cal) return;
    
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            switch (event.scancode) {
                case KEY_ESC:
                    cal->running = false;
                    break;
                case KEY_LEFT:
                    if (cal->selected_day > 1) cal->selected_day--;
                    else calendar_prev_month(cal);
                    break;
                case KEY_RIGHT:
                    if (cal->selected_day < days_in_month(cal->year, cal->month))
                        cal->selected_day++;
                    else calendar_next_month(cal);
                    break;
                case KEY_UP:
                    if (cal->selected_day > 7) cal->selected_day -= 7;
                    break;
                case KEY_DOWN:
                    if (cal->selected_day + 7 <= days_in_month(cal->year, cal->month))
                        cal->selected_day += 7;
                    break;
                default:
                    if (event.ascii == '<' || event.ascii == ',') calendar_prev_month(cal);
                    else if (event.ascii == '>' || event.ascii == '.') calendar_next_month(cal);
                    break;
            }
        }
    }
    
    calendar_draw(cal);
}

static void run_calendar(void) {
    calendar_t *cal = calendar_create();
    if (!cal) return;
    while (cal->running) {
        calendar_update(cal);
        timer_sleep_ms(16);
    }
    calendar_destroy(cal);
}

/* ==================== CLOCK ==================== */
clock_t *clock_create(void) {
    clock_t *clk = (clock_t *)kcalloc(1, sizeof(clock_t));
    if (!clk) return NULL;
    
    clk->window = javier_window_create("Clock", 200, 200, 200, 100, WIN_DEFAULT);
    clk->show_seconds = true;
    clk->is_24h = true;
    clk->running = true;
    
    return clk;
}

void clock_destroy(clock_t *clk) {
    if (clk) {
        if (clk->window) javier_window_destroy(clk->window);
        kfree(clk);
    }
}

void clock_draw(clock_t *clk) {
    if (!clk || !clk->window) return;
    
    int x = clk->window->bounds.x + clk->window->client.x;
    int y = clk->window->bounds.y + clk->window->client.y;
    int w = clk->window->client.width;
    int h = clk->window->client.height;
    
    fb_fill_rect(x, y, w, h, RGB(20, 20, 25));
    
    char time_str[16];
    if (clk->show_seconds) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                clk->hours, clk->minutes, clk->seconds);
    } else {
        snprintf(time_str, sizeof(time_str), "%02d:%02d",
                clk->hours, clk->minutes);
    }
    
    /* Large time display */
    int tw = strlen(time_str) * 16;
    fb_draw_string(x + (w - tw) / 2, y + h / 2 - 8, time_str, APP_ACCENT, 0);
}

void clock_update(clock_t *clk) {
    if (!clk) return;
    
    /* Update time from system ticks */
    uint64_t ticks = timer_get_ticks();
    uint64_t total_secs = ticks / 18;  /* ~18.2 ticks per second */
    clk->seconds = total_secs % 60;
    clk->minutes = (total_secs / 60) % 60;
    clk->hours = (total_secs / 3600) % 24;
    
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed && event.scancode == KEY_ESC) {
            clk->running = false;
        }
    }
    
    clock_draw(clk);
}

static void run_clock(void) {
    clock_t *clk = clock_create();
    if (!clk) return;
    while (clk->running) {
        clock_update(clk);
        timer_sleep_ms(100);
    }
    clock_destroy(clk);
}

/* Stub implementations for remaining apps */
static void run_notes(void) {
    javier_window_t *w = javier_window_create("Notes", 100, 100, 300, 400, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 10,
                      "Notes App - Press ESC to close", APP_TEXT, 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}

static void run_todolist(void) {
    javier_window_t *w = javier_window_create("Todo List", 100, 100, 300, 400, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 10,
                      "Todo List - Press ESC to close", APP_TEXT, 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}

static void run_filemanager(void) {
    javier_window_t *w = javier_window_create("File Manager", 50, 50, 600, 400, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 10,
                      "File Manager - Press ESC to close", APP_TEXT, 0);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 30,
                      "/ (root)", APP_ACCENT, 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}

static void run_sysmonitor(void) {
    javier_window_t *w = javier_window_create("System Monitor", 50, 50, 500, 400, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 10,
                      "System Monitor", APP_TEXT, 0);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 40,
                      "CPU: AtomOS Kernel", APP_TEXT, 0);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 60,
                      "Memory: Running smoothly", APP_TEXT, 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(100);
    }
    javier_window_destroy(w);
}

static void run_settings(void) {
    javier_window_t *w = javier_window_create("Settings", 100, 100, 400, 300, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 10,
                      "Settings - Press ESC to close", APP_TEXT, 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}

static void run_paint(void) {
    javier_window_t *w = javier_window_create("Paint", 50, 50, 640, 480, WIN_DEFAULT);
    bool running = true;
    int cx = 320, cy = 240;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, RGB(255, 255, 255));
        fb_fill_rect(w->bounds.x + w->client.x + cx - 2, 
                    w->bounds.y + w->client.y + cy - 2, 4, 4, RGB(0, 0, 0));
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed) {
                if (e.scancode == KEY_ESC) running = false;
                else if (e.scancode == KEY_UP) cy -= 5;
                else if (e.scancode == KEY_DOWN) cy += 5;
                else if (e.scancode == KEY_LEFT) cx -= 5;
                else if (e.scancode == KEY_RIGHT) cx += 5;
            }
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}

static void run_colorpicker(void) {
    javier_window_t *w = javier_window_create("Color Picker", 100, 100, 300, 350, WIN_DEFAULT);
    int r = 128, g = 128, b = 128;
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        fb_fill_rect(w->bounds.x + w->client.x + 50, w->bounds.y + w->client.y + 50,
                    200, 100, RGB(r, g, b));
        char buf[64];
        snprintf(buf, sizeof(buf), "R: %d  G: %d  B: %d", r, g, b);
        fb_draw_string(w->bounds.x + w->client.x + 80, w->bounds.y + w->client.y + 180, buf, APP_TEXT, 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed) {
                if (e.scancode == KEY_ESC) running = false;
                else if (e.ascii == 'r') r = (r + 10) % 256;
                else if (e.ascii == 'g') g = (g + 10) % 256;
                else if (e.ascii == 'b') b = (b + 10) % 256;
            }
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}

static void run_timer(void) {
    javier_window_t *w = javier_window_create("Timer", 150, 150, 250, 150, WIN_DEFAULT);
    int secs = 60;
    bool counting = false;
    bool running = true;
    uint64_t last = timer_get_ticks();
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%02d", secs / 60, secs % 60);
        fb_draw_string(w->bounds.x + w->client.x + 80, w->bounds.y + w->client.y + 40, buf, APP_ACCENT, 0);
        fb_draw_string(w->bounds.x + w->client.x + 50, w->bounds.y + w->client.y + 80,
                      counting ? "[SPACE] Stop" : "[SPACE] Start", APP_TEXT, 0);
        if (counting) {
            uint64_t now = timer_get_ticks();
            if (now - last >= 18) {
                if (secs > 0) secs--;
                last = now;
            }
        }
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed) {
                if (e.scancode == KEY_ESC) running = false;
                else if (e.ascii == ' ') counting = !counting;
                else if (e.ascii == 'r') { secs = 60; counting = false; }
            }
        }
        timer_sleep_ms(50);
    }
    javier_window_destroy(w);
}

static void run_stopwatch(void) {
    javier_window_t *w = javier_window_create("Stopwatch", 150, 150, 250, 150, WIN_DEFAULT);
    uint64_t elapsed = 0;
    bool counting = false;
    bool running = true;
    uint64_t last = timer_get_ticks();
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        int secs = elapsed / 18;
        int ms = (elapsed % 18) * 55;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%02d.%02d", secs / 60, secs % 60, ms / 10);
        fb_draw_string(w->bounds.x + w->client.x + 60, w->bounds.y + w->client.y + 40, buf, APP_ACCENT, 0);
        fb_draw_string(w->bounds.x + w->client.x + 50, w->bounds.y + w->client.y + 80,
                      counting ? "[SPACE] Stop" : "[SPACE] Start", APP_TEXT, 0);
        if (counting) {
            uint64_t now = timer_get_ticks();
            elapsed += now - last;
            last = now;
        } else {
            last = timer_get_ticks();
        }
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed) {
                if (e.scancode == KEY_ESC) running = false;
                else if (e.ascii == ' ') counting = !counting;
                else if (e.ascii == 'r') { elapsed = 0; counting = false; }
            }
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}

/* More stub apps... */
static void run_alarm(void) { run_clock(); }
static void run_converter(void) { run_calculator(); }
static void run_weather(void) {
    javier_window_t *w = javier_window_create("Weather", 100, 100, 300, 200, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, RGB(135, 206, 235));
        fb_draw_string(w->bounds.x + w->client.x + 100, w->bounds.y + w->client.y + 30, "Sunny", APP_TEXT, 0);
        fb_draw_string(w->bounds.x + w->client.x + 100, w->bounds.y + w->client.y + 60, "72F / 22C", APP_TEXT, 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(100);
    }
    javier_window_destroy(w);
}

static void run_dictionary(void) { run_notes(); }
static void run_scicalc(void) { run_calculator(); }
static void run_contacts(void) { run_notes(); }
static void run_hexeditor(void) { run_text_editor(); }
static void run_codeeditor(void) { run_text_editor(); }
static void run_videoplayer(void) { run_sysmonitor(); }
static void run_recorder(void) { run_sysmonitor(); }
static void run_netmonitor(void) { run_sysmonitor(); }
static void run_fontviewer(void) { run_sysmonitor(); }

static void run_sysinfo(void) {
    javier_window_t *w = javier_window_create("System Info", 100, 100, 400, 300, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        int y = w->bounds.y + w->client.y + 20;
        int x = w->bounds.x + w->client.x + 20;
        fb_draw_string(x, y, "AtomOS System Information", APP_ACCENT, 0); y += 30;
        fb_draw_string(x, y, "OS: AtomOS v1.0", APP_TEXT, 0); y += 20;
        fb_draw_string(x, y, "Kernel: Javier 1.0", APP_TEXT, 0); y += 20;
        fb_draw_string(x, y, "Architecture: x86", APP_TEXT, 0); y += 20;
        fb_draw_string(x, y, "Memory: Smart RAM Enabled", APP_TEXT, 0); y += 20;
        fb_draw_string(x, y, "Graphics: VBE Framebuffer", APP_TEXT, 0); y += 20;
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(100);
    }
    javier_window_destroy(w);
}

static void run_about(void) {
    javier_window_t *w = javier_window_create("About AtomOS", 150, 150, 350, 250, WIN_DEFAULT);
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        int y = w->bounds.y + w->client.y + 30;
        int x = w->bounds.x + w->client.x + (w->client.width - 100) / 2;
        fb_draw_string(x, y, "AtomOS", RGB(255, 150, 50), 0); y += 30;
        fb_draw_string(x - 40, y, "Javier Desktop Environment", APP_TEXT, 0); y += 30;
        fb_draw_string(x - 20, y, "Version 1.0", APP_TEXT, 0); y += 40;
        fb_draw_string(x - 60, y, "A complete OS from scratch", RGB(150, 150, 150), 0); y += 20;
        fb_draw_string(x - 30, y, "Written in pure C", RGB(150, 150, 150), 0);
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed && e.scancode == KEY_ESC) running = false;
        }
        timer_sleep_ms(100);
    }
    javier_window_destroy(w);
}

static void run_launcher(void) {
    javier_window_t *w = javier_window_create("App Launcher", 100, 50, 500, 450, WIN_DEFAULT);
    int selected = 0;
    int scroll = 0;
    bool running = true;
    while (running) {
        fb_fill_rect(w->bounds.x + w->client.x, w->bounds.y + w->client.y,
                    w->client.width, w->client.height, APP_BG);
        fb_draw_string(w->bounds.x + w->client.x + 10, w->bounds.y + w->client.y + 10,
                      "Applications", APP_ACCENT, 0);
        
        int y = w->bounds.y + w->client.y + 40;
        int visible = 15;
        for (int i = scroll; i < app_count && i < scroll + visible; i++) {
            color_t bg = (i == selected) ? APP_ACCENT : APP_PANEL;
            fb_fill_rect(w->bounds.x + w->client.x + 5, y, w->client.width - 10, 24, bg);
            fb_draw_string(w->bounds.x + w->client.x + 15, y + 4, app_list[i].name, APP_TEXT, 0);
            fb_draw_string(w->bounds.x + w->client.x + 150, y + 4, app_list[i].description, 
                          RGB(150, 150, 150), 0);
            y += 26;
        }
        
        if (keyboard_has_key()) {
            key_event_t e = keyboard_get_key();
            if (e.pressed) {
                if (e.scancode == KEY_ESC) running = false;
                else if (e.scancode == KEY_UP && selected > 0) {
                    selected--;
                    if (selected < scroll) scroll = selected;
                }
                else if (e.scancode == KEY_DOWN && selected < app_count - 1) {
                    selected++;
                    if (selected >= scroll + visible) scroll = selected - visible + 1;
                }
                else if (e.scancode == KEY_ENTER) {
                    running = false;
                    javier_window_destroy(w);
                    if (app_list[selected].run) app_list[selected].run();
                    return;
                }
            }
        }
        timer_sleep_ms(16);
    }
    javier_window_destroy(w);
}
