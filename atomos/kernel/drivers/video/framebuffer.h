/*
 * AtomOS - Framebuffer Graphics Driver
 * VESA VBE graphics mode support for Javier UI
 */

#ifndef _ATOMOS_FRAMEBUFFER_H
#define _ATOMOS_FRAMEBUFFER_H

#include "../../include/types.h"

/* Color type (32-bit ARGB) */
typedef uint32_t color_t;

/* Color macros */
#define RGB(r, g, b)        (0xFF000000 | ((r) << 16) | ((g) << 8) | (b))
#define RGBA(r, g, b, a)    (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define GET_R(c)            (((c) >> 16) & 0xFF)
#define GET_G(c)            (((c) >> 8) & 0xFF)
#define GET_B(c)            ((c) & 0xFF)
#define GET_A(c)            (((c) >> 24) & 0xFF)

/* Javier UI color palette */
#define COLOR_TRANSPARENT   0x00000000
#define COLOR_BLACK         RGB(0, 0, 0)
#define COLOR_WHITE         RGB(255, 255, 255)
#define COLOR_BG_DARK       RGB(18, 18, 24)
#define COLOR_BG_LIGHT      RGB(28, 28, 38)
#define COLOR_ACCENT        RGB(88, 166, 255)
#define COLOR_ACCENT_DARK   RGB(56, 116, 203)
#define COLOR_TEXT          RGB(230, 230, 240)
#define COLOR_TEXT_DIM      RGB(150, 150, 170)
#define COLOR_BORDER        RGB(48, 48, 68)
#define COLOR_SUCCESS       RGB(80, 200, 120)
#define COLOR_WARNING       RGB(255, 190, 80)
#define COLOR_ERROR         RGB(255, 85, 85)
#define COLOR_WINDOW_BG     RGB(24, 24, 32)
#define COLOR_TASKBAR       RGB(12, 12, 18)

/* Point structure */
typedef struct {
    int32_t x;
    int32_t y;
} point_t;

/* Rectangle structure */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} rect_t;

/* Framebuffer info */
typedef struct {
    uint32_t *buffer;       /* Framebuffer memory */
    uint32_t width;         /* Screen width in pixels */
    uint32_t height;        /* Screen height in pixels */
    uint32_t pitch;         /* Bytes per scanline */
    uint32_t bpp;           /* Bits per pixel */
    uint32_t size;          /* Total framebuffer size */
} framebuffer_t;

/* Function declarations */
void fb_init(uint32_t addr, uint32_t width, uint32_t height, 
             uint32_t pitch, uint32_t bpp);
framebuffer_t *fb_get_info(void);

/* Basic drawing */
void fb_clear(color_t color);
void fb_put_pixel(int x, int y, color_t color);
color_t fb_get_pixel(int x, int y);

/* Shape drawing */
void fb_draw_line(int x1, int y1, int x2, int y2, color_t color);
void fb_draw_rect(int x, int y, int w, int h, color_t color);
void fb_fill_rect(int x, int y, int w, int h, color_t color);
void fb_draw_circle(int cx, int cy, int r, color_t color);
void fb_fill_circle(int cx, int cy, int r, color_t color);
void fb_draw_rounded_rect(int x, int y, int w, int h, int r, color_t color);
void fb_fill_rounded_rect(int x, int y, int w, int h, int r, color_t color);

/* Gradient */
void fb_fill_gradient_v(int x, int y, int w, int h, color_t c1, color_t c2);
void fb_fill_gradient_h(int x, int y, int w, int h, color_t c1, color_t c2);

/* Text rendering */
void fb_draw_char(int x, int y, char c, color_t fg, color_t bg);
void fb_draw_string(int x, int y, const char *str, color_t fg, color_t bg);
int fb_string_width(const char *str);
int fb_char_height(void);

/* Bitmap operations */
void fb_blit(int x, int y, uint32_t *bitmap, int w, int h);
void fb_blit_alpha(int x, int y, uint32_t *bitmap, int w, int h);

/* Double buffering */
void fb_swap_buffers(void);
uint32_t *fb_get_back_buffer(void);

/* Clipping */
void fb_set_clip(int x, int y, int w, int h);
void fb_reset_clip(void);

#endif /* _ATOMOS_FRAMEBUFFER_H */
