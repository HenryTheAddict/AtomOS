/*
 * AtomOS - PS/2 Keyboard Driver
 * Handles keyboard input via PS/2 controller
 */

#ifndef _ATOMOS_KEYBOARD_H
#define _ATOMOS_KEYBOARD_H

#include "../../include/types.h"

/* Keyboard ports */
#define KB_DATA_PORT    0x60
#define KB_STATUS_PORT  0x64
#define KB_COMMAND_PORT 0x64

/* Special keys */
typedef enum {
    KEY_NONE = 0,
    KEY_ESC = 0x01,
    KEY_BACKSPACE = 0x0E,
    KEY_TAB = 0x0F,
    KEY_ENTER = 0x1C,
    KEY_LCTRL = 0x1D,
    KEY_LSHIFT = 0x2A,
    KEY_RSHIFT = 0x36,
    KEY_LALT = 0x38,
    KEY_CAPSLOCK = 0x3A,
    KEY_F1 = 0x3B,
    KEY_F2 = 0x3C,
    KEY_F3 = 0x3D,
    KEY_F4 = 0x3E,
    KEY_F5 = 0x3F,
    KEY_F6 = 0x40,
    KEY_F7 = 0x41,
    KEY_F8 = 0x42,
    KEY_F9 = 0x43,
    KEY_F10 = 0x44,
    KEY_NUMLOCK = 0x45,
    KEY_SCROLLLOCK = 0x46,
    KEY_HOME = 0x47,
    KEY_UP = 0x48,
    KEY_PAGEUP = 0x49,
    KEY_LEFT = 0x4B,
    KEY_RIGHT = 0x4D,
    KEY_END = 0x4F,
    KEY_DOWN = 0x50,
    KEY_PAGEDOWN = 0x51,
    KEY_INSERT = 0x52,
    KEY_DELETE = 0x53,
    KEY_F11 = 0x57,
    KEY_F12 = 0x58,
} key_code_t;

/* Keyboard modifiers */
typedef enum {
    MOD_NONE = 0,
    MOD_SHIFT = 1,
    MOD_CTRL = 2,
    MOD_ALT = 4,
    MOD_CAPS = 8,
    MOD_NUM = 16,
} key_mod_t;

/* Key event structure */
typedef struct {
    uint8_t scancode;       /* Raw scancode */
    char ascii;             /* ASCII character (0 if special key) */
    key_code_t keycode;     /* Special key code */
    uint8_t modifiers;      /* Active modifiers */
    bool pressed;           /* Key pressed (vs released) */
} key_event_t;

/* Keyboard callback function */
typedef void (*keyboard_callback_t)(key_event_t *event);

/* Function declarations */
void keyboard_init(void);
void keyboard_set_callback(keyboard_callback_t callback);

/* Polling interface */
bool keyboard_has_key(void);
key_event_t keyboard_get_key(void);
char keyboard_get_char(void);

/* LED control */
void keyboard_set_leds(bool scroll, bool num, bool caps);

/* Key state */
bool keyboard_is_key_pressed(uint8_t scancode);
uint8_t keyboard_get_modifiers(void);

#endif /* _ATOMOS_KEYBOARD_H */
