/*
 * AtomOS - PS/2 Keyboard Driver Implementation
 */

#include "keyboard.h"
#include "../../include/kernel.h"
#include "../../arch/x86/idt.h"

/* Keyboard state */
static uint8_t key_states[256];
static uint8_t modifiers = 0;
static keyboard_callback_t kb_callback = NULL;

/* Key buffer */
#define KEY_BUFFER_SIZE 64
static key_event_t key_buffer[KEY_BUFFER_SIZE];
static volatile int key_buffer_head = 0;
static volatile int key_buffer_tail = 0;

/* US keyboard layout - lowercase */
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

/* US keyboard layout - uppercase/shifted */
static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

/*
 * Add key to buffer
 */
static void keyboard_buffer_push(key_event_t *event) {
    int next = (key_buffer_head + 1) % KEY_BUFFER_SIZE;
    if (next != key_buffer_tail) {
        key_buffer[key_buffer_head] = *event;
        key_buffer_head = next;
    }
}

/*
 * Get key from buffer
 */
static bool keyboard_buffer_pop(key_event_t *event) {
    if (key_buffer_head == key_buffer_tail) {
        return false;
    }
    *event = key_buffer[key_buffer_tail];
    key_buffer_tail = (key_buffer_tail + 1) % KEY_BUFFER_SIZE;
    return true;
}

/*
 * Process scancode
 */
static void keyboard_process_scancode(uint8_t scancode) {
    key_event_t event;
    event.scancode = scancode;
    event.ascii = 0;
    event.keycode = KEY_NONE;
    event.modifiers = modifiers;
    event.pressed = !(scancode & 0x80);
    
    /* Get actual scancode (remove release bit) */
    uint8_t code = scancode & 0x7F;
    
    /* Update key state */
    key_states[code] = event.pressed;
    
    /* Handle modifier keys */
    switch (code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            if (event.pressed) modifiers |= MOD_SHIFT;
            else modifiers &= ~MOD_SHIFT;
            break;
            
        case KEY_LCTRL:
            if (event.pressed) modifiers |= MOD_CTRL;
            else modifiers &= ~MOD_CTRL;
            break;
            
        case KEY_LALT:
            if (event.pressed) modifiers |= MOD_ALT;
            else modifiers &= ~MOD_ALT;
            break;
            
        case KEY_CAPSLOCK:
            if (event.pressed) modifiers ^= MOD_CAPS;
            break;
            
        case KEY_NUMLOCK:
            if (event.pressed) modifiers ^= MOD_NUM;
            break;
    }
    
    event.modifiers = modifiers;
    
    /* Convert to ASCII for key press */
    if (event.pressed && code < sizeof(scancode_to_ascii)) {
        bool shifted = (modifiers & MOD_SHIFT) != 0;
        bool caps = (modifiers & MOD_CAPS) != 0;
        
        /* For letters, caps lock inverts shift */
        char ch = scancode_to_ascii[code];
        if (ch >= 'a' && ch <= 'z') {
            if (shifted ^ caps) {
                event.ascii = scancode_to_ascii_shift[code];
            } else {
                event.ascii = ch;
            }
        } else if (shifted) {
            event.ascii = scancode_to_ascii_shift[code];
        } else {
            event.ascii = ch;
        }
    }
    
    /* Set keycode for special keys */
    if (code <= KEY_F12) {
        event.keycode = (key_code_t)code;
    }
    
    /* Add to buffer */
    keyboard_buffer_push(&event);
    
    /* Call callback if registered */
    if (kb_callback) {
        kb_callback(&event);
    }
}

/*
 * Keyboard interrupt handler
 */
static void keyboard_handler(registers_t *regs) {
    (void)regs;  /* Unused parameter */
    uint8_t scancode = inb(KB_DATA_PORT);
    
    keyboard_process_scancode(scancode);
}

/*
 * Wait for keyboard controller
 */
static void keyboard_wait_write(void) {
    while (inb(KB_STATUS_PORT) & 0x02);
}

static void keyboard_wait_read(void) {
    while (!(inb(KB_STATUS_PORT) & 0x01));
}

/*
 * Send command to keyboard
 */
static void keyboard_send_command(uint8_t cmd) {
    keyboard_wait_write();
    outb(KB_DATA_PORT, cmd);
}

/*
 * Initialize keyboard
 */
void keyboard_init(void) {
    /* Clear key states */
    memset(key_states, 0, sizeof(key_states));
    modifiers = 0;
    
    /* Register interrupt handler */
    register_interrupt_handler(IRQ1, keyboard_handler);
    
    /* Enable keyboard */
    keyboard_wait_write();
    outb(KB_COMMAND_PORT, 0xAE);
    
    /* Set scan code set 1 */
    keyboard_send_command(0xF0);
    keyboard_wait_read();
    inb(KB_DATA_PORT);  /* ACK */
    keyboard_send_command(0x01);
    keyboard_wait_read();
    inb(KB_DATA_PORT);  /* ACK */
    
    /* Enable scanning */
    keyboard_send_command(0xF4);
    keyboard_wait_read();
    inb(KB_DATA_PORT);  /* ACK */
    
    kprintf("Keyboard initialized\n");
}

/*
 * Set keyboard callback
 */
void keyboard_set_callback(keyboard_callback_t callback) {
    kb_callback = callback;
}

/*
 * Check if key is available
 */
bool keyboard_has_key(void) {
    return key_buffer_head != key_buffer_tail;
}

/*
 * Get next key event
 */
key_event_t keyboard_get_key(void) {
    key_event_t event = {0};
    keyboard_buffer_pop(&event);
    return event;
}

/*
 * Get next ASCII character (blocking)
 */
char keyboard_get_char(void) {
    key_event_t event;
    
    while (1) {
        if (keyboard_buffer_pop(&event)) {
            if (event.pressed && event.ascii) {
                return event.ascii;
            }
        }
        hlt();  /* Wait for interrupt */
    }
}

/*
 * Check if a specific key is pressed
 */
bool keyboard_is_key_pressed(uint8_t scancode) {
    return key_states[scancode & 0x7F];
}

/*
 * Get current modifiers
 */
uint8_t keyboard_get_modifiers(void) {
    return modifiers;
}

/*
 * Set keyboard LEDs
 */
void keyboard_set_leds(bool scroll, bool num, bool caps) {
    uint8_t leds = 0;
    if (scroll) leds |= 1;
    if (num) leds |= 2;
    if (caps) leds |= 4;
    
    keyboard_send_command(0xED);
    keyboard_wait_read();
    inb(KB_DATA_PORT);
    keyboard_send_command(leds);
    keyboard_wait_read();
    inb(KB_DATA_PORT);
}
