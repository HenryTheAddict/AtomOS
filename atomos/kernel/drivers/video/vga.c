/*
 * AtomOS - VGA Text Mode Implementation
 */

#include "vga.h"
#include "../../include/kernel.h"
#include <stdarg.h>

/* VGA state */
static uint16_t *vga_buffer;
static int vga_cursor_x;
static int vga_cursor_y;
static uint8_t vga_color_attr;

/*
 * Update hardware cursor position
 */
static void vga_update_cursor(void) {
    uint16_t pos = vga_cursor_y * VGA_WIDTH + vga_cursor_x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/*
 * Initialize VGA text mode
 */
void vga_init(void) {
    vga_buffer = (uint16_t *)VGA_MEMORY;
    vga_cursor_x = 0;
    vga_cursor_y = 0;
    vga_color_attr = VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK);
    
    vga_enable_cursor(14, 15);
    vga_clear();
}

/*
 * Clear the screen
 */
void vga_clear(void) {
    uint16_t blank = VGA_ENTRY(' ', vga_color_attr);
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    
    vga_cursor_x = 0;
    vga_cursor_y = 0;
    vga_update_cursor();
}

/*
 * Set text color
 */
void vga_set_color(uint8_t color) {
    vga_color_attr = color;
}

/*
 * Set cursor position
 */
void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_cursor_x = x;
        vga_cursor_y = y;
        vga_update_cursor();
    }
}

/*
 * Get cursor position
 */
void vga_get_cursor(int *x, int *y) {
    if (x) *x = vga_cursor_x;
    if (y) *y = vga_cursor_y;
}

/*
 * Enable cursor with specified shape
 */
void vga_enable_cursor(uint8_t start, uint8_t end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | start);
    
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

/*
 * Disable cursor
 */
void vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

/*
 * Scroll screen up by one line
 */
void vga_scroll(void) {
    /* Move all lines up */
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    
    /* Clear last line */
    uint16_t blank = VGA_ENTRY(' ', vga_color_attr);
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = blank;
    }
    
    vga_cursor_y = VGA_HEIGHT - 1;
}

/*
 * Put a character on screen
 */
void vga_putchar(char c) {
    switch (c) {
        case '\n':
            vga_cursor_x = 0;
            vga_cursor_y++;
            break;
            
        case '\r':
            vga_cursor_x = 0;
            break;
            
        case '\t':
            vga_cursor_x = (vga_cursor_x + 8) & ~7;
            break;
            
        case '\b':
            if (vga_cursor_x > 0) {
                vga_cursor_x--;
                vga_buffer[vga_cursor_y * VGA_WIDTH + vga_cursor_x] = 
                    VGA_ENTRY(' ', vga_color_attr);
            }
            break;
            
        default:
            if (c >= ' ') {
                vga_buffer[vga_cursor_y * VGA_WIDTH + vga_cursor_x] = 
                    VGA_ENTRY(c, vga_color_attr);
                vga_cursor_x++;
            }
            break;
    }
    
    /* Handle line wrap */
    if (vga_cursor_x >= VGA_WIDTH) {
        vga_cursor_x = 0;
        vga_cursor_y++;
    }
    
    /* Handle scroll */
    if (vga_cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }
    
    vga_update_cursor();
}

/*
 * Put a string on screen
 */
void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/*
 * Convert integer to string
 */
static char *itoa(int value, char *str, int base) {
    char *p = str;
    char *p1, *p2;
    unsigned int uvalue;
    char digits[] = "0123456789ABCDEF";
    
    /* Handle negative for decimal */
    if (value < 0 && base == 10) {
        *p++ = '-';
        uvalue = -value;
    } else {
        uvalue = (unsigned int)value;
    }
    
    /* Convert */
    char *start = p;
    do {
        *p++ = digits[uvalue % base];
        uvalue /= base;
    } while (uvalue);
    
    *p = '\0';
    
    /* Reverse */
    p1 = start;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
    
    return str;
}

/*
 * Formatted print to VGA
 */
void vga_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char buf[32];
    
    while (*fmt) {
        if (*fmt != '%') {
            vga_putchar(*fmt++);
            continue;
        }
        
        fmt++;
        
        switch (*fmt) {
            case 'd':
            case 'i':
                itoa(va_arg(args, int), buf, 10);
                vga_puts(buf);
                break;
                
            case 'u':
                itoa(va_arg(args, unsigned int), buf, 10);
                vga_puts(buf);
                break;
                
            case 'x':
            case 'X':
                itoa(va_arg(args, unsigned int), buf, 16);
                vga_puts(buf);
                break;
                
            case 'p':
                vga_puts("0x");
                itoa(va_arg(args, unsigned int), buf, 16);
                vga_puts(buf);
                break;
                
            case 's': {
                char *s = va_arg(args, char *);
                vga_puts(s ? s : "(null)");
                break;
            }
                
            case 'c':
                vga_putchar((char)va_arg(args, int));
                break;
                
            case '%':
                vga_putchar('%');
                break;
                
            default:
                vga_putchar('%');
                vga_putchar(*fmt);
                break;
        }
        
        fmt++;
    }
    
    va_end(args);
}
