/*
 * Javier - AtomOS Desktop Environment
 * A clean, modern graphical interface
 */

#ifndef _JAVIER_H
#define _JAVIER_H

#include "../../kernel/include/types.h"
#include "../../kernel/drivers/video/framebuffer.h"

/* Javier version */
#define JAVIER_VERSION_MAJOR    1
#define JAVIER_VERSION_MINOR    0
#define JAVIER_VERSION_STRING   "1.0"

/* Window flags */
#define WIN_NORMAL      0x00
#define WIN_TITLEBAR    0x01
#define WIN_RESIZABLE   0x02
#define WIN_MOVABLE     0x04
#define WIN_CLOSABLE    0x08
#define WIN_MINIMIZABLE 0x10
#define WIN_MAXIMIZABLE 0x20
#define WIN_BORDER      0x40
#define WIN_SHADOW      0x80
#define WIN_DEFAULT     (WIN_TITLEBAR | WIN_MOVABLE | WIN_CLOSABLE | WIN_BORDER | WIN_SHADOW)

/* Window states */
#define WIN_STATE_NORMAL    0
#define WIN_STATE_MINIMIZED 1
#define WIN_STATE_MAXIMIZED 2
#define WIN_STATE_HIDDEN    3

/* Event types */
typedef enum {
    EVENT_NONE = 0,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MOUSE_CLICK,
    EVENT_MOUSE_DBLCLICK,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_KEY_CHAR,
    EVENT_WINDOW_CLOSE,
    EVENT_WINDOW_RESIZE,
    EVENT_WINDOW_MOVE,
    EVENT_WINDOW_FOCUS,
    EVENT_WINDOW_BLUR,
    EVENT_PAINT,
    EVENT_TIMER,
} event_type_t;

/* Mouse buttons */
#define MOUSE_LEFT      1
#define MOUSE_RIGHT     2
#define MOUSE_MIDDLE    4

/* Event structure */
typedef struct {
    event_type_t type;
    uint32_t target;        /* Window ID */
    union {
        struct {
            int x, y;
            int dx, dy;
            uint8_t buttons;
        } mouse;
        struct {
            uint8_t scancode;
            char ascii;
            uint8_t modifiers;
        } key;
        struct {
            int x, y;
            int width, height;
        } window;
    };
} javier_event_t;

/* Forward declarations */
struct javier_window;
struct javier_widget;

/* Event handler callback */
typedef bool (*event_handler_t)(struct javier_window *win, javier_event_t *event);

/* Widget types */
typedef enum {
    WIDGET_NONE = 0,
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_TEXTBOX,
    WIDGET_CHECKBOX,
    WIDGET_RADIO,
    WIDGET_LISTBOX,
    WIDGET_COMBOBOX,
    WIDGET_SCROLLBAR,
    WIDGET_PROGRESS,
    WIDGET_SLIDER,
    WIDGET_IMAGE,
    WIDGET_PANEL,
    WIDGET_MENU,
    WIDGET_MENUITEM,
    WIDGET_TOOLBAR,
    WIDGET_STATUSBAR,
    WIDGET_TAB,
    WIDGET_CANVAS,
} widget_type_t;

/* Widget structure */
typedef struct javier_widget {
    widget_type_t type;
    uint32_t id;
    char text[256];
    rect_t bounds;
    bool visible;
    bool enabled;
    bool focused;
    bool hovered;
    color_t fg_color;
    color_t bg_color;
    void *data;
    event_handler_t handler;
    struct javier_widget *parent;
    struct javier_widget *children;
    struct javier_widget *next;
} javier_widget_t;

/* Window structure */
typedef struct javier_window {
    uint32_t id;
    char title[256];
    rect_t bounds;
    rect_t client;          /* Client area (excluding title bar, borders) */
    uint32_t flags;
    uint8_t state;
    bool visible;
    bool focused;
    color_t bg_color;
    uint32_t *buffer;       /* Window buffer for compositing */
    javier_widget_t *widgets;
    event_handler_t handler;
    struct javier_window *next;
    struct javier_window *prev;
} javier_window_t;

/* Desktop structure */
typedef struct {
    color_t bg_color;
    uint32_t *wallpaper;
    int wallpaper_width;
    int wallpaper_height;
    javier_window_t *windows;
    javier_window_t *focused;
    javier_window_t *dragging;
    int drag_offset_x;
    int drag_offset_y;
} javier_desktop_t;

/* Mouse state */
typedef struct {
    int x, y;
    int prev_x, prev_y;
    uint8_t buttons;
    uint8_t prev_buttons;
} javier_mouse_t;

/* Core functions */
void javier_init(void);
void javier_run(void);
void javier_shutdown(void);

/* Desktop */
void javier_desktop_set_wallpaper(uint32_t *image, int w, int h);
void javier_desktop_draw(void);

/* Window management */
javier_window_t *javier_window_create(const char *title, int x, int y, 
                                       int width, int height, uint32_t flags);
void javier_window_destroy(javier_window_t *win);
void javier_window_show(javier_window_t *win);
void javier_window_hide(javier_window_t *win);
void javier_window_focus(javier_window_t *win);
void javier_window_move(javier_window_t *win, int x, int y);
void javier_window_resize(javier_window_t *win, int width, int height);
void javier_window_set_title(javier_window_t *win, const char *title);
void javier_window_draw(javier_window_t *win);
void javier_window_invalidate(javier_window_t *win);

/* Widget creation */
javier_widget_t *javier_label_create(javier_window_t *win, const char *text,
                                      int x, int y, int w, int h);
javier_widget_t *javier_button_create(javier_window_t *win, const char *text,
                                       int x, int y, int w, int h);
javier_widget_t *javier_textbox_create(javier_window_t *win, 
                                        int x, int y, int w, int h);
javier_widget_t *javier_checkbox_create(javier_window_t *win, const char *text,
                                         int x, int y);
javier_widget_t *javier_progress_create(javier_window_t *win,
                                         int x, int y, int w, int h);

/* Widget operations */
void javier_widget_set_text(javier_widget_t *widget, const char *text);
const char *javier_widget_get_text(javier_widget_t *widget);
void javier_widget_set_handler(javier_widget_t *widget, event_handler_t handler);
void javier_widget_draw(javier_widget_t *widget);

/* Event handling */
void javier_process_events(void);
bool javier_dispatch_event(javier_event_t *event);

/* Mouse */
void javier_mouse_update(int x, int y, uint8_t buttons);
void javier_mouse_draw(void);

/* Taskbar */
void javier_taskbar_init(void);
void javier_taskbar_draw(void);
void javier_taskbar_add_window(javier_window_t *win);
void javier_taskbar_remove_window(javier_window_t *win);

/* Compositor */
void javier_composite(void);
void javier_redraw(void);

#endif /* _JAVIER_H */
