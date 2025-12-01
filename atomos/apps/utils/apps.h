/*
 * AtomOS Applications Suite
 * 50 utility applications
 */

#ifndef _ATOMOS_APPS_H
#define _ATOMOS_APPS_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"

/* Application info */
typedef struct {
    const char *name;
    const char *description;
    const char *icon;
    void (*run)(void);
} app_info_t;

/* List of all applications */
extern app_info_t app_list[];
extern int app_count;

/* ============ PRODUCTIVITY APPS ============ */

/* Text Editor */
typedef struct {
    javier_window_t *window;
    char *buffer;
    size_t buf_size;
    size_t buf_len;
    int cursor_line;
    int cursor_col;
    int scroll_y;
    char filename[256];
    bool modified;
    bool running;
} text_editor_t;

text_editor_t *text_editor_create(void);
void text_editor_destroy(text_editor_t *ed);
void text_editor_draw(text_editor_t *ed);
void text_editor_update(text_editor_t *ed);
void text_editor_open(text_editor_t *ed, const char *path);
void text_editor_save(text_editor_t *ed);
void text_editor_insert(text_editor_t *ed, char c);
void text_editor_delete(text_editor_t *ed);

/* Calculator */
typedef struct {
    javier_window_t *window;
    char display[64];
    double value1;
    double value2;
    char op;
    bool new_number;
    bool running;
} calculator_t;

calculator_t *calculator_create(void);
void calculator_destroy(calculator_t *calc);
void calculator_draw(calculator_t *calc);
void calculator_update(calculator_t *calc);
void calculator_press(calculator_t *calc, char key);
void calculator_clear(calculator_t *calc);
void calculator_equals(calculator_t *calc);

/* Calendar */
typedef struct {
    javier_window_t *window;
    int year;
    int month;
    int day;
    int selected_day;
    bool running;
} calendar_t;

calendar_t *calendar_create(void);
void calendar_destroy(calendar_t *cal);
void calendar_draw(calendar_t *cal);
void calendar_update(calendar_t *cal);
void calendar_next_month(calendar_t *cal);
void calendar_prev_month(calendar_t *cal);

/* Clock */
typedef struct {
    javier_window_t *window;
    int hours;
    int minutes;
    int seconds;
    bool show_seconds;
    bool is_24h;
    bool running;
} clock_t;

clock_t *clock_create(void);
void clock_destroy(clock_t *clk);
void clock_draw(clock_t *clk);
void clock_update(clock_t *clk);

/* Notes */
typedef struct {
    javier_window_t *window;
    char notes[20][256];
    int note_count;
    int selected;
    char input[256];
    int input_len;
    bool editing;
    bool running;
} notes_t;

notes_t *notes_create(void);
void notes_destroy(notes_t *notes);
void notes_draw(notes_t *notes);
void notes_update(notes_t *notes);
void notes_add(notes_t *notes, const char *text);
void notes_delete(notes_t *notes, int index);

/* Todo List */
typedef struct {
    javier_window_t *window;
    struct {
        char text[128];
        bool done;
    } items[50];
    int item_count;
    int selected;
    char input[128];
    int input_len;
    bool adding;
    bool running;
} todolist_t;

todolist_t *todolist_create(void);
void todolist_destroy(todolist_t *todo);
void todolist_draw(todolist_t *todo);
void todolist_update(todolist_t *todo);
void todolist_add(todolist_t *todo, const char *text);
void todolist_toggle(todolist_t *todo, int index);
void todolist_delete(todolist_t *todo, int index);

/* ============ SYSTEM APPS ============ */

/* File Manager */
typedef struct {
    javier_window_t *window;
    char current_path[256];
    struct {
        char name[64];
        bool is_dir;
        uint32_t size;
    } entries[100];
    int entry_count;
    int selected;
    int scroll;
    bool running;
} filemanager_t;

filemanager_t *filemanager_create(void);
void filemanager_destroy(filemanager_t *fm);
void filemanager_draw(filemanager_t *fm);
void filemanager_update(filemanager_t *fm);
void filemanager_navigate(filemanager_t *fm, const char *path);
void filemanager_open(filemanager_t *fm);
void filemanager_delete(filemanager_t *fm);

/* System Monitor */
typedef struct {
    javier_window_t *window;
    int cpu_usage;
    uint32_t mem_total;
    uint32_t mem_used;
    uint32_t mem_free;
    struct {
        int pid;
        char name[32];
        int cpu;
        uint32_t mem;
    } processes[32];
    int process_count;
    int selected;
    bool running;
} sysmonitor_t;

sysmonitor_t *sysmonitor_create(void);
void sysmonitor_destroy(sysmonitor_t *sm);
void sysmonitor_draw(sysmonitor_t *sm);
void sysmonitor_update(sysmonitor_t *sm);

/* Settings */
typedef struct {
    javier_window_t *window;
    int category;
    int selected;
    struct {
        const char *name;
        int type;
        int value;
    } options[20];
    int option_count;
    bool running;
} settings_t;

settings_t *settings_create(void);
void settings_destroy(settings_t *set);
void settings_draw(settings_t *set);
void settings_update(settings_t *set);

/* ============ GRAPHICS APPS ============ */

/* Paint */
typedef struct {
    javier_window_t *window;
    uint32_t *canvas;
    int canvas_w;
    int canvas_h;
    int cursor_x;
    int cursor_y;
    color_t fg_color;
    color_t bg_color;
    int brush_size;
    int tool;  /* 0=pencil, 1=brush, 2=line, 3=rect, 4=circle, 5=fill, 6=eraser */
    bool drawing;
    int start_x, start_y;
    bool running;
} paint_t;

paint_t *paint_create(void);
void paint_destroy(paint_t *p);
void paint_draw(paint_t *p);
void paint_update(paint_t *p);
void paint_clear(paint_t *p);
void paint_set_color(paint_t *p, color_t color);
void paint_set_tool(paint_t *p, int tool);

/* Color Picker */
typedef struct {
    javier_window_t *window;
    int r, g, b;
    int h, s, v;
    int cursor_x, cursor_y;
    bool running;
} colorpicker_t;

colorpicker_t *colorpicker_create(void);
void colorpicker_destroy(colorpicker_t *cp);
void colorpicker_draw(colorpicker_t *cp);
void colorpicker_update(colorpicker_t *cp);

/* Screenshot */
void screenshot_take(void);
void screenshot_save(const char *path);

/* ============ UTILITY APPS ============ */

/* Timer */
typedef struct {
    javier_window_t *window;
    int hours;
    int minutes;
    int seconds;
    bool running_timer;
    bool countdown;
    uint64_t start_time;
    uint64_t elapsed;
    bool running;
} timer_app_t;

timer_app_t *timer_app_create(void);
void timer_app_destroy(timer_app_t *t);
void timer_app_draw(timer_app_t *t);
void timer_app_update(timer_app_t *t);
void timer_app_start(timer_app_t *t);
void timer_app_stop(timer_app_t *t);
void timer_app_reset(timer_app_t *t);

/* Stopwatch */
typedef struct {
    javier_window_t *window;
    uint64_t start_time;
    uint64_t elapsed;
    uint64_t laps[20];
    int lap_count;
    bool running_sw;
    bool running;
} stopwatch_t;

stopwatch_t *stopwatch_create(void);
void stopwatch_destroy(stopwatch_t *sw);
void stopwatch_draw(stopwatch_t *sw);
void stopwatch_update(stopwatch_t *sw);
void stopwatch_start(stopwatch_t *sw);
void stopwatch_stop(stopwatch_t *sw);
void stopwatch_lap(stopwatch_t *sw);
void stopwatch_reset(stopwatch_t *sw);

/* Alarm */
typedef struct {
    javier_window_t *window;
    struct {
        int hour;
        int minute;
        bool enabled;
        char label[32];
    } alarms[10];
    int alarm_count;
    int selected;
    bool editing;
    bool running;
} alarm_t;

alarm_t *alarm_create(void);
void alarm_destroy(alarm_t *a);
void alarm_draw(alarm_t *a);
void alarm_update(alarm_t *a);
void alarm_add(alarm_t *a, int hour, int minute, const char *label);
void alarm_toggle(alarm_t *a, int index);

/* Unit Converter */
typedef struct {
    javier_window_t *window;
    int category;  /* 0=length, 1=weight, 2=temp, 3=volume, 4=area */
    int from_unit;
    int to_unit;
    double value;
    double result;
    char input[32];
    bool running;
} converter_t;

converter_t *converter_create(void);
void converter_destroy(converter_t *c);
void converter_draw(converter_t *c);
void converter_update(converter_t *c);
void converter_convert(converter_t *c);

/* Weather */
typedef struct {
    javier_window_t *window;
    char city[64];
    int temp;
    int humidity;
    int wind;
    char condition[32];
    int forecast[7];
    bool running;
} weather_t;

weather_t *weather_create(void);
void weather_destroy(weather_t *w);
void weather_draw(weather_t *w);
void weather_update(weather_t *w);

/* ============ EDUCATIONAL APPS ============ */

/* Dictionary */
typedef struct {
    javier_window_t *window;
    char search[64];
    char word[64];
    char definition[512];
    bool found;
    bool running;
} dictionary_t;

dictionary_t *dictionary_create(void);
void dictionary_destroy(dictionary_t *d);
void dictionary_draw(dictionary_t *d);
void dictionary_update(dictionary_t *d);
void dictionary_lookup(dictionary_t *d, const char *word);

/* Scientific Calculator */
typedef struct {
    javier_window_t *window;
    char display[128];
    char input[128];
    double memory;
    int mode;  /* 0=deg, 1=rad */
    bool running;
} scicalc_t;

scicalc_t *scicalc_create(void);
void scicalc_destroy(scicalc_t *sc);
void scicalc_draw(scicalc_t *sc);
void scicalc_update(scicalc_t *sc);

/* ============ COMMUNICATION APPS ============ */

/* Contacts */
typedef struct {
    javier_window_t *window;
    struct {
        char name[64];
        char phone[32];
        char email[64];
    } contacts[100];
    int contact_count;
    int selected;
    bool editing;
    bool running;
} contacts_t;

contacts_t *contacts_create(void);
void contacts_destroy(contacts_t *c);
void contacts_draw(contacts_t *c);
void contacts_update(contacts_t *c);
void contacts_add(contacts_t *c, const char *name, const char *phone, const char *email);
void contacts_delete(contacts_t *c, int index);

/* ============ CODE APPS ============ */

/* Hex Editor */
typedef struct {
    javier_window_t *window;
    uint8_t *data;
    size_t data_size;
    size_t offset;
    int cursor;
    bool ascii_mode;
    char filename[256];
    bool running;
} hexeditor_t;

hexeditor_t *hexeditor_create(void);
void hexeditor_destroy(hexeditor_t *he);
void hexeditor_draw(hexeditor_t *he);
void hexeditor_update(hexeditor_t *he);
void hexeditor_open(hexeditor_t *he, const char *path);
void hexeditor_save(hexeditor_t *he);

/* Code Editor */
typedef struct {
    javier_window_t *window;
    char *buffer;
    size_t buf_size;
    size_t buf_len;
    int cursor_line;
    int cursor_col;
    int scroll_y;
    char filename[256];
    int syntax;  /* 0=none, 1=c, 2=javi, 3=asm */
    bool modified;
    bool running;
} codeeditor_t;

codeeditor_t *codeeditor_create(void);
void codeeditor_destroy(codeeditor_t *ce);
void codeeditor_draw(codeeditor_t *ce);
void codeeditor_update(codeeditor_t *ce);
void codeeditor_open(codeeditor_t *ce, const char *path);
void codeeditor_save(codeeditor_t *ce);

/* ============ MEDIA APPS ============ */

/* Video Player (basic frame viewer) */
typedef struct {
    javier_window_t *window;
    int frame;
    int total_frames;
    bool playing;
    bool running;
} videoplayer_t;

videoplayer_t *videoplayer_create(void);
void videoplayer_destroy(videoplayer_t *vp);
void videoplayer_draw(videoplayer_t *vp);
void videoplayer_update(videoplayer_t *vp);

/* Audio Recorder (concept) */
typedef struct {
    javier_window_t *window;
    bool recording;
    uint64_t record_start;
    int record_length;
    bool running;
} recorder_t;

recorder_t *recorder_create(void);
void recorder_destroy(recorder_t *r);
void recorder_draw(recorder_t *r);
void recorder_update(recorder_t *r);

/* ============ NETWORK APPS ============ */

/* Network Monitor */
typedef struct {
    javier_window_t *window;
    struct {
        char name[16];
        char ip[16];
        bool connected;
        uint32_t rx_bytes;
        uint32_t tx_bytes;
    } interfaces[4];
    int iface_count;
    bool running;
} netmonitor_t;

netmonitor_t *netmonitor_create(void);
void netmonitor_destroy(netmonitor_t *nm);
void netmonitor_draw(netmonitor_t *nm);
void netmonitor_update(netmonitor_t *nm);

/* ============ MISC APPS ============ */

/* Font Viewer */
typedef struct {
    javier_window_t *window;
    int font_size;
    char preview_text[256];
    bool running;
} fontviewer_t;

fontviewer_t *fontviewer_create(void);
void fontviewer_destroy(fontviewer_t *fv);
void fontviewer_draw(fontviewer_t *fv);
void fontviewer_update(fontviewer_t *fv);

/* System Info */
typedef struct {
    javier_window_t *window;
    char os_name[64];
    char os_version[32];
    char cpu_info[64];
    uint32_t ram_total;
    uint32_t uptime;
    bool running;
} sysinfo_t;

sysinfo_t *sysinfo_create(void);
void sysinfo_destroy(sysinfo_t *si);
void sysinfo_draw(sysinfo_t *si);
void sysinfo_update(sysinfo_t *si);

/* About */
typedef struct {
    javier_window_t *window;
    bool running;
} about_t;

about_t *about_create(void);
void about_destroy(about_t *a);
void about_draw(about_t *a);
void about_update(about_t *a);

/* App Launcher */
typedef struct {
    javier_window_t *window;
    int selected;
    int scroll;
    char search[64];
    bool running;
} launcher_t;

launcher_t *launcher_create(void);
void launcher_destroy(launcher_t *l);
void launcher_draw(launcher_t *l);
void launcher_update(launcher_t *l);

#endif /* _ATOMOS_APPS_H */
