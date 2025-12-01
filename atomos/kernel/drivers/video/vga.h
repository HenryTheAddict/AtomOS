/*
 * AtomOS - VGA Text Mode Driver
 * Basic text output for early boot and console
 */

#ifndef _ATOMOS_VGA_H
#define _ATOMOS_VGA_H

#include "../../include/types.h"

/* VGA text mode constants */
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_MEMORY      0xB8000

/* VGA colors */
typedef enum {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14,
    VGA_WHITE = 15
} vga_color_t;

/* Create VGA color attribute */
#define VGA_COLOR(fg, bg)   ((bg) << 4 | (fg))
#define VGA_ENTRY(c, color) ((uint16_t)(c) | (uint16_t)(color) << 8)

/* Function declarations */
void vga_init(void);
void vga_clear(void);
void vga_set_color(uint8_t color);
void vga_set_cursor(int x, int y);
void vga_get_cursor(int *x, int *y);
void vga_enable_cursor(uint8_t start, uint8_t end);
void vga_disable_cursor(void);
void vga_putchar(char c);
void vga_puts(const char *str);
void vga_scroll(void);

/* Formatted output */
void vga_printf(const char *fmt, ...);

#endif /* _ATOMOS_VGA_H */
