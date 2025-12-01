/*
 * Javier - AtomOS Desktop Environment Implementation
 * Window management, compositing, and event handling
 */

#include "javier.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/timer.h"

/* Global state */
static javier_desktop_t desktop;
static javier_mouse_t mouse;
static framebuffer_t *fb;
static uint32_t next_window_id = 1;
static uint32_t next_widget_id = 1;
static bool running = false;

/* Taskbar configuration */
#define TASKBAR_HEIGHT  40
#define TITLEBAR_HEIGHT 28
#define BORDER_WIDTH    1
#define SHADOW_SIZE     6

/* Window chrome drawing */
static void draw_window_chrome(javier_window_t *win) {
    int x = win->bounds.x;
    int y = win->bounds.y;
    int w = win->bounds.width;
    int h = win->bounds.height;
    
    /* Shadow */
    if (win->flags & WIN_SHADOW) {
        for (int i = 0; i < SHADOW_SIZE; i++) {
            uint8_t alpha = 40 - i * 6;
            color_t shadow = RGBA(0, 0, 0, alpha);
            fb_draw_rect(x + i + 2, y + i + 2, w, h, shadow);
        }
    }
    
    /* Window background */
    fb_fill_rounded_rect(x, y, w, h, 8, COLOR_WINDOW_BG);
    
    /* Title bar */
    if (win->flags & WIN_TITLEBAR) {
        color_t title_bg = win->focused ? COLOR_ACCENT : COLOR_BG_LIGHT;
        fb_fill_rounded_rect(x, y, w, TITLEBAR_HEIGHT, 8, title_bg);
        fb_fill_rect(x, y + TITLEBAR_HEIGHT - 8, w, 8, title_bg);
        
        /* Title text */
        int text_x = x + 12;
        int text_y = y + (TITLEBAR_HEIGHT - fb_char_height()) / 2;
        fb_draw_string(text_x, text_y, win->title, COLOR_TEXT, COLOR_TRANSPARENT);
        
        /* Close button */
        if (win->flags & WIN_CLOSABLE) {
            int btn_x = x + w - 32;
            int btn_y = y + 6;
            int btn_size = 16;
            
            fb_fill_circle(btn_x + btn_size/2, btn_y + btn_size/2, 
                          btn_size/2, COLOR_ERROR);
            fb_draw_line(btn_x + 4, btn_y + 4, 
                        btn_x + btn_size - 4, btn_y + btn_size - 4, COLOR_WHITE);
            fb_draw_line(btn_x + btn_size - 4, btn_y + 4, 
                        btn_x + 4, btn_y + btn_size - 4, COLOR_WHITE);
        }
        
        /* Minimize button */
        if (win->flags & WIN_MINIMIZABLE) {
            int btn_x = x + w - 56;
            int btn_y = y + 6;
            int btn_size = 16;
            
            fb_fill_circle(btn_x + btn_size/2, btn_y + btn_size/2, 
                          btn_size/2, COLOR_WARNING);
            fb_draw_line(btn_x + 4, btn_y + btn_size/2, 
                        btn_x + btn_size - 4, btn_y + btn_size/2, COLOR_BLACK);
        }
        
        /* Maximize button */
        if (win->flags & WIN_MAXIMIZABLE) {
            int btn_x = x + w - 80;
            int btn_y = y + 6;
            int btn_size = 16;
            
            fb_fill_circle(btn_x + btn_size/2, btn_y + btn_size/2, 
                          btn_size/2, COLOR_SUCCESS);
            fb_draw_rect(btn_x + 4, btn_y + 4, btn_size - 8, btn_size - 8, COLOR_BLACK);
        }
    }
    
    /* Border */
    if (win->flags & WIN_BORDER) {
        color_t border = win->focused ? COLOR_ACCENT : COLOR_BORDER;
        fb_draw_rounded_rect(x, y, w, h, 8, border);
    }
}

/*
 * Initialize Javier desktop environment
 */
void javier_init(void) {
    fb = fb_get_info();
    
    /* Initialize desktop */
    memset(&desktop, 0, sizeof(desktop));
    desktop.bg_color = COLOR_BG_DARK;
    
    /* Initialize mouse */
    memset(&mouse, 0, sizeof(mouse));
    mouse.x = fb->width / 2;
    mouse.y = fb->height / 2;
    
    /* Initialize taskbar */
    javier_taskbar_init();
    
    running = true;
    
    kprintf("Javier desktop environment initialized\n");
}

/*
 * Create a new window
 */
javier_window_t *javier_window_create(const char *title, int x, int y, 
                                       int width, int height, uint32_t flags) {
    javier_window_t *win = (javier_window_t *)kcalloc(1, sizeof(javier_window_t));
    if (!win) return NULL;
    
    win->id = next_window_id++;
    strncpy(win->title, title, sizeof(win->title) - 1);
    win->bounds.x = x;
    win->bounds.y = y;
    win->bounds.width = width;
    win->bounds.height = height;
    win->flags = flags;
    win->state = WIN_STATE_NORMAL;
    win->visible = true;
    win->focused = false;
    win->bg_color = COLOR_WINDOW_BG;
    
    /* Calculate client area */
    int title_h = (flags & WIN_TITLEBAR) ? TITLEBAR_HEIGHT : 0;
    int border = (flags & WIN_BORDER) ? BORDER_WIDTH : 0;
    win->client.x = border;
    win->client.y = title_h;
    win->client.width = width - 2 * border;
    win->client.height = height - title_h - border;
    
    /* Add to window list */
    if (desktop.windows) {
        javier_window_t *last = desktop.windows;
        while (last->next) last = last->next;
        last->next = win;
        win->prev = last;
    } else {
        desktop.windows = win;
    }
    
    /* Focus new window */
    javier_window_focus(win);
    
    /* Add to taskbar */
    javier_taskbar_add_window(win);
    
    return win;
}

/*
 * Destroy a window
 */
void javier_window_destroy(javier_window_t *win) {
    if (!win) return;
    
    /* Remove from taskbar */
    javier_taskbar_remove_window(win);
    
    /* Remove from window list */
    if (win->prev) {
        win->prev->next = win->next;
    } else {
        desktop.windows = win->next;
    }
    if (win->next) {
        win->next->prev = win->prev;
    }
    
    /* Focus another window */
    if (desktop.focused == win) {
        desktop.focused = desktop.windows;
        if (desktop.focused) {
            desktop.focused->focused = true;
        }
    }
    
    /* Free widgets */
    javier_widget_t *widget = win->widgets;
    while (widget) {
        javier_widget_t *next = widget->next;
        kfree(widget);
        widget = next;
    }
    
    kfree(win);
}

/*
 * Show a window
 */
void javier_window_show(javier_window_t *win) {
    if (win) {
        win->visible = true;
        win->state = WIN_STATE_NORMAL;
    }
}

/*
 * Hide a window
 */
void javier_window_hide(javier_window_t *win) {
    if (win) {
        win->visible = false;
    }
}

/*
 * Focus a window
 */
void javier_window_focus(javier_window_t *win) {
    if (!win) return;
    
    /* Unfocus current */
    if (desktop.focused && desktop.focused != win) {
        desktop.focused->focused = false;
    }
    
    /* Focus new window */
    win->focused = true;
    desktop.focused = win;
    
    /* Move to front */
    if (win->prev) {
        /* Remove from current position */
        win->prev->next = win->next;
        if (win->next) win->next->prev = win->prev;
        
        /* Add to end */
        javier_window_t *last = desktop.windows;
        while (last->next) last = last->next;
        last->next = win;
        win->prev = last;
        win->next = NULL;
    }
}

/*
 * Move a window
 */
void javier_window_move(javier_window_t *win, int x, int y) {
    if (win) {
        win->bounds.x = x;
        win->bounds.y = y;
    }
}

/*
 * Resize a window
 */
void javier_window_resize(javier_window_t *win, int width, int height) {
    if (!win) return;
    
    win->bounds.width = width;
    win->bounds.height = height;
    
    /* Recalculate client area */
    int title_h = (win->flags & WIN_TITLEBAR) ? TITLEBAR_HEIGHT : 0;
    int border = (win->flags & WIN_BORDER) ? BORDER_WIDTH : 0;
    win->client.width = width - 2 * border;
    win->client.height = height - title_h - border;
}

/*
 * Draw a window
 */
void javier_window_draw(javier_window_t *win) {
    if (!win || !win->visible || win->state == WIN_STATE_MINIMIZED) {
        return;
    }
    
    /* Draw chrome (title bar, borders, shadow) */
    draw_window_chrome(win);
    
    /* Draw client area background */
    int cx = win->bounds.x + win->client.x;
    int cy = win->bounds.y + win->client.y;
    int cw = win->client.width;
    int ch = win->client.height;
    
    fb_fill_rect(cx, cy, cw, ch, win->bg_color);
    
    /* Draw widgets */
    javier_widget_t *widget = win->widgets;
    while (widget) {
        if (widget->visible) {
            javier_widget_draw(widget);
        }
        widget = widget->next;
    }
}

/*
 * Draw a widget
 */
void javier_widget_draw(javier_widget_t *widget) {
    if (!widget) return;
    
    int x = widget->bounds.x;
    int y = widget->bounds.y;
    int w = widget->bounds.width;
    int h = widget->bounds.height;
    
    switch (widget->type) {
        case WIDGET_LABEL:
            fb_draw_string(x, y, widget->text, widget->fg_color, COLOR_TRANSPARENT);
            break;
            
        case WIDGET_BUTTON: {
            color_t bg = widget->hovered ? COLOR_ACCENT : COLOR_BG_LIGHT;
            if (!widget->enabled) bg = COLOR_BG_DARK;
            
            fb_fill_rounded_rect(x, y, w, h, 4, bg);
            fb_draw_rounded_rect(x, y, w, h, 4, COLOR_BORDER);
            
            int tw = fb_string_width(widget->text);
            int th = fb_char_height();
            fb_draw_string(x + (w - tw) / 2, y + (h - th) / 2, 
                          widget->text, widget->fg_color, COLOR_TRANSPARENT);
            break;
        }
        
        case WIDGET_TEXTBOX: {
            fb_fill_rect(x, y, w, h, COLOR_BG_DARK);
            fb_draw_rect(x, y, w, h, widget->focused ? COLOR_ACCENT : COLOR_BORDER);
            fb_draw_string(x + 4, y + (h - fb_char_height()) / 2,
                          widget->text, widget->fg_color, COLOR_TRANSPARENT);
            break;
        }
        
        case WIDGET_CHECKBOX: {
            int box_size = 16;
            fb_fill_rect(x, y, box_size, box_size, COLOR_BG_DARK);
            fb_draw_rect(x, y, box_size, box_size, COLOR_BORDER);
            
            if (widget->data) {
                fb_draw_line(x + 3, y + 8, x + 6, y + 12, COLOR_ACCENT);
                fb_draw_line(x + 6, y + 12, x + 13, y + 4, COLOR_ACCENT);
            }
            
            fb_draw_string(x + box_size + 8, y + 1, widget->text,
                          widget->fg_color, COLOR_TRANSPARENT);
            break;
        }
        
        case WIDGET_PROGRESS: {
            fb_fill_rect(x, y, w, h, COLOR_BG_DARK);
            fb_draw_rect(x, y, w, h, COLOR_BORDER);
            
            int value = (int)(uintptr_t)widget->data;
            int fill_w = (w - 4) * value / 100;
            fb_fill_rect(x + 2, y + 2, fill_w, h - 4, COLOR_ACCENT);
            break;
        }
        
        default:
            break;
    }
}

/*
 * Create a label widget
 */
javier_widget_t *javier_label_create(javier_window_t *win, const char *text,
                                      int x, int y, int w, int h) {
    javier_widget_t *widget = (javier_widget_t *)kcalloc(1, sizeof(javier_widget_t));
    if (!widget) return NULL;
    
    widget->type = WIDGET_LABEL;
    widget->id = next_widget_id++;
    strncpy(widget->text, text, sizeof(widget->text) - 1);
    widget->bounds.x = win->bounds.x + win->client.x + x;
    widget->bounds.y = win->bounds.y + win->client.y + y;
    widget->bounds.width = w;
    widget->bounds.height = h;
    widget->visible = true;
    widget->enabled = true;
    widget->fg_color = COLOR_TEXT;
    widget->bg_color = COLOR_TRANSPARENT;
    
    /* Add to window */
    widget->next = win->widgets;
    win->widgets = widget;
    
    return widget;
}

/*
 * Create a button widget
 */
javier_widget_t *javier_button_create(javier_window_t *win, const char *text,
                                       int x, int y, int w, int h) {
    javier_widget_t *widget = javier_label_create(win, text, x, y, w, h);
    if (widget) {
        widget->type = WIDGET_BUTTON;
        widget->bg_color = COLOR_BG_LIGHT;
    }
    return widget;
}

/*
 * Create a textbox widget
 */
javier_widget_t *javier_textbox_create(javier_window_t *win, 
                                        int x, int y, int w, int h) {
    javier_widget_t *widget = javier_label_create(win, "", x, y, w, h);
    if (widget) {
        widget->type = WIDGET_TEXTBOX;
    }
    return widget;
}

/*
 * Create a checkbox widget
 */
javier_widget_t *javier_checkbox_create(javier_window_t *win, const char *text,
                                         int x, int y) {
    javier_widget_t *widget = javier_label_create(win, text, x, y, 
                                                   fb_string_width(text) + 24, 16);
    if (widget) {
        widget->type = WIDGET_CHECKBOX;
    }
    return widget;
}

/*
 * Create a progress bar widget
 */
javier_widget_t *javier_progress_create(javier_window_t *win,
                                         int x, int y, int w, int h) {
    javier_widget_t *widget = javier_label_create(win, "", x, y, w, h);
    if (widget) {
        widget->type = WIDGET_PROGRESS;
        widget->data = (void *)0;
    }
    return widget;
}

/*
 * Set widget text
 */
void javier_widget_set_text(javier_widget_t *widget, const char *text) {
    if (widget) {
        strncpy(widget->text, text, sizeof(widget->text) - 1);
    }
}

/*
 * Get widget text
 */
const char *javier_widget_get_text(javier_widget_t *widget) {
    return widget ? widget->text : NULL;
}

/*
 * Set widget event handler
 */
void javier_widget_set_handler(javier_widget_t *widget, event_handler_t handler) {
    if (widget) {
        widget->handler = handler;
    }
}

/*
 * Draw taskbar
 */
static javier_window_t *taskbar_buttons[32];
static int taskbar_button_count = 0;

void javier_taskbar_init(void) {
    taskbar_button_count = 0;
    memset(taskbar_buttons, 0, sizeof(taskbar_buttons));
}

void javier_taskbar_draw(void) {
    int y = fb->height - TASKBAR_HEIGHT;
    
    /* Background */
    fb_fill_rect(0, y, fb->width, TASKBAR_HEIGHT, COLOR_TASKBAR);
    fb_draw_line(0, y, fb->width, y, COLOR_BORDER);
    
    /* Start button */
    fb_fill_rounded_rect(8, y + 6, 80, TASKBAR_HEIGHT - 12, 4, COLOR_ACCENT);
    fb_draw_string(20, y + 12, "AtomOS", COLOR_WHITE, COLOR_TRANSPARENT);
    
    /* Window buttons */
    int btn_x = 100;
    for (int i = 0; i < taskbar_button_count && i < 32; i++) {
        javier_window_t *win = taskbar_buttons[i];
        if (!win) continue;
        
        color_t bg = (win == desktop.focused) ? COLOR_ACCENT_DARK : COLOR_BG_LIGHT;
        fb_fill_rounded_rect(btn_x, y + 6, 120, TASKBAR_HEIGHT - 12, 4, bg);
        
        char truncated[16];
        strncpy(truncated, win->title, 15);
        truncated[15] = '\0';
        fb_draw_string(btn_x + 8, y + 12, truncated, COLOR_TEXT, COLOR_TRANSPARENT);
        
        btn_x += 128;
    }
    
    /* Clock */
    uint64_t uptime = timer_get_uptime();
    int hours = (uptime / 3600) % 24;
    int mins = (uptime / 60) % 60;
    char time_str[16];
    time_str[0] = '0' + hours / 10;
    time_str[1] = '0' + hours % 10;
    time_str[2] = ':';
    time_str[3] = '0' + mins / 10;
    time_str[4] = '0' + mins % 10;
    time_str[5] = '\0';
    
    fb_draw_string(fb->width - 60, y + 12, time_str, COLOR_TEXT, COLOR_TRANSPARENT);
}

void javier_taskbar_add_window(javier_window_t *win) {
    if (taskbar_button_count < 32) {
        taskbar_buttons[taskbar_button_count++] = win;
    }
}

void javier_taskbar_remove_window(javier_window_t *win) {
    for (int i = 0; i < taskbar_button_count; i++) {
        if (taskbar_buttons[i] == win) {
            for (int j = i; j < taskbar_button_count - 1; j++) {
                taskbar_buttons[j] = taskbar_buttons[j + 1];
            }
            taskbar_button_count--;
            break;
        }
    }
}

/*
 * Draw desktop background
 */
void javier_desktop_draw(void) {
    if (desktop.wallpaper) {
        fb_blit(0, 0, desktop.wallpaper, desktop.wallpaper_width, 
                desktop.wallpaper_height);
    } else {
        /* Gradient background */
        fb_fill_gradient_v(0, 0, fb->width, fb->height - TASKBAR_HEIGHT,
                          COLOR_BG_DARK, RGB(30, 30, 45));
    }
}

/*
 * Draw mouse cursor
 */
void javier_mouse_draw(void) {
    int x = mouse.x;
    int y = mouse.y;
    
    /* Simple arrow cursor */
    color_t cursor_color = COLOR_WHITE;
    color_t shadow_color = RGB(0, 0, 0);
    
    /* Shadow */
    for (int i = 0; i < 12; i++) {
        fb_draw_line(x + 1, y + 1, x + 1, y + i + 2, shadow_color);
    }
    fb_draw_line(x + 1, y + 12, x + 5, y + 9, shadow_color);
    fb_draw_line(x + 5, y + 9, x + 8, y + 15, shadow_color);
    
    /* Cursor */
    for (int i = 0; i < 12; i++) {
        fb_draw_line(x, y, x, y + i + 1, cursor_color);
    }
    fb_draw_line(x, y + 12, x + 4, y + 8, cursor_color);
    fb_draw_line(x + 4, y + 8, x + 7, y + 14, cursor_color);
}

/*
 * Update mouse position
 */
void javier_mouse_update(int x, int y, uint8_t buttons) {
    mouse.prev_x = mouse.x;
    mouse.prev_y = mouse.y;
    mouse.prev_buttons = mouse.buttons;
    
    mouse.x = x;
    mouse.y = y;
    mouse.buttons = buttons;
    
    /* Clamp to screen */
    if (mouse.x < 0) mouse.x = 0;
    if (mouse.y < 0) mouse.y = 0;
    if (mouse.x >= (int)fb->width) mouse.x = fb->width - 1;
    if (mouse.y >= (int)fb->height) mouse.y = fb->height - 1;
}

/*
 * Composite all elements
 */
void javier_composite(void) {
    /* Draw desktop */
    javier_desktop_draw();
    
    /* Draw windows (back to front) */
    javier_window_t *win = desktop.windows;
    while (win) {
        javier_window_draw(win);
        win = win->next;
    }
    
    /* Draw taskbar */
    javier_taskbar_draw();
    
    /* Draw cursor */
    javier_mouse_draw();
    
    /* Swap buffers */
    fb_swap_buffers();
}

/*
 * Redraw everything
 */
void javier_redraw(void) {
    javier_composite();
}

/*
 * Process events
 */
void javier_process_events(void) {
    /* Process keyboard events */
    while (keyboard_has_key()) {
        key_event_t key = keyboard_get_key();
        
        if (key.pressed && desktop.focused) {
            javier_event_t event;
            event.type = EVENT_KEY_DOWN;
            event.target = desktop.focused->id;
            event.key.scancode = key.scancode;
            event.key.ascii = key.ascii;
            event.key.modifiers = key.modifiers;
            
            if (desktop.focused->handler) {
                desktop.focused->handler(desktop.focused, &event);
            }
        }
    }
    
    /* TODO: Process mouse events when mouse driver is implemented */
}

/*
 * Main loop
 */
void javier_run(void) {
    while (running) {
        javier_process_events();
        javier_redraw();
        
        /* Small delay to prevent 100% CPU usage */
        timer_sleep_ms(16);  /* ~60 FPS */
    }
}

/*
 * Shutdown Javier
 */
void javier_shutdown(void) {
    running = false;
    
    /* Destroy all windows */
    while (desktop.windows) {
        javier_window_destroy(desktop.windows);
    }
}
