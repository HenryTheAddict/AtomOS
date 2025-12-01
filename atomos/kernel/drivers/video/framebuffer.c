/*
 * AtomOS - Framebuffer Implementation
 * Graphics primitives for Javier UI
 */

#include "framebuffer.h"
#include "../../include/kernel.h"
#include "../../mm/pmm.h"

/* Framebuffer state */
static framebuffer_t fb;
static uint32_t *back_buffer = NULL;

/* Clipping rectangle */
static rect_t clip_rect;
static bool clipping_enabled = false;

/* 8x16 font data (built-in bitmap font) */
static const uint8_t font_8x16[256][16] = {
    /* ASCII 32-126 basic characters - simplified representation */
    /* Space (32) */
    [32] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ! (33) */
    [33] = {0x00,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
    /* " (34) */
    [34] = {0x00,0x66,0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 0-9 */
    [48] = {0x00,0x00,0x38,0x6C,0xC6,0xC6,0xD6,0xD6,0xC6,0xC6,0x6C,0x38,0x00,0x00,0x00,0x00},
    [49] = {0x00,0x00,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00,0x00},
    [50] = {0x00,0x00,0x7C,0xC6,0x06,0x0C,0x18,0x30,0x60,0xC0,0xC6,0xFE,0x00,0x00,0x00,0x00},
    [51] = {0x00,0x00,0x7C,0xC6,0x06,0x06,0x3C,0x06,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,0x00},
    [52] = {0x00,0x00,0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x0C,0x1E,0x00,0x00,0x00,0x00},
    [53] = {0x00,0x00,0xFE,0xC0,0xC0,0xC0,0xFC,0x06,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,0x00},
    [54] = {0x00,0x00,0x38,0x60,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
    [55] = {0x00,0x00,0xFE,0xC6,0x06,0x06,0x0C,0x18,0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00},
    [56] = {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
    [57] = {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7E,0x06,0x06,0x06,0x0C,0x78,0x00,0x00,0x00,0x00},
    /* A-Z */
    [65] = {0x00,0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00},
    [66] = {0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0x66,0xFC,0x00,0x00,0x00,0x00},
    [67] = {0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xC0,0xC0,0xC2,0x66,0x3C,0x00,0x00,0x00,0x00},
    [68] = {0x00,0x00,0xF8,0x6C,0x66,0x66,0x66,0x66,0x66,0x66,0x6C,0xF8,0x00,0x00,0x00,0x00},
    [69] = {0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x62,0x66,0xFE,0x00,0x00,0x00,0x00},
    [70] = {0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00},
    /* ... more characters would be added ... */
    /* a-z lowercase */
    [97] = {0x00,0x00,0x00,0x00,0x00,0x78,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00},
    [98] = {0x00,0x00,0xE0,0x60,0x60,0x78,0x6C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00},
    [99] = {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00,0x00,0x00,0x00},
    [100] = {0x00,0x00,0x1C,0x0C,0x0C,0x3C,0x6C,0xCC,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00},
    [101] = {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xFE,0xC0,0xC0,0xC6,0x7C,0x00,0x00,0x00,0x00},
    [102] = {0x00,0x00,0x38,0x6C,0x64,0x60,0xF0,0x60,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00},
};

/*
 * Blend two colors with alpha
 */
static color_t blend_colors(color_t fg, color_t bg) {
    uint8_t alpha = GET_A(fg);
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;
    
    uint8_t r = (GET_R(fg) * alpha + GET_R(bg) * (255 - alpha)) / 255;
    uint8_t g = (GET_G(fg) * alpha + GET_G(bg) * (255 - alpha)) / 255;
    uint8_t b = (GET_B(fg) * alpha + GET_B(bg) * (255 - alpha)) / 255;
    
    return RGB(r, g, b);
}

/*
 * Check if point is within clip region
 */
static bool in_clip(int x, int y) {
    if (!clipping_enabled) return true;
    return x >= clip_rect.x && x < clip_rect.x + clip_rect.width &&
           y >= clip_rect.y && y < clip_rect.y + clip_rect.height;
}

/*
 * Initialize framebuffer
 */
void fb_init(uint32_t addr, uint32_t width, uint32_t height, 
             uint32_t pitch, uint32_t bpp) {
    fb.buffer = (uint32_t *)addr;
    fb.width = width;
    fb.height = height;
    fb.pitch = pitch;
    fb.bpp = bpp;
    fb.size = pitch * height;
    
    /* Allocate back buffer for double buffering */
    back_buffer = (uint32_t *)pmm_alloc_pages(fb.size / PAGE_SIZE + 1);
    
    /* Reset clipping */
    fb_reset_clip();
    
    kprintf("Framebuffer: %dx%d, %d bpp\n", width, height, bpp);
}

/*
 * Get framebuffer info
 */
framebuffer_t *fb_get_info(void) {
    return &fb;
}

/*
 * Clear screen with color
 */
void fb_clear(color_t color) {
    uint32_t *buf = back_buffer ? back_buffer : fb.buffer;
    for (uint32_t i = 0; i < fb.width * fb.height; i++) {
        buf[i] = color;
    }
}

/*
 * Put a single pixel
 */
void fb_put_pixel(int x, int y, color_t color) {
    if (x < 0 || x >= (int)fb.width || y < 0 || y >= (int)fb.height) return;
    if (!in_clip(x, y)) return;
    
    uint32_t *buf = back_buffer ? back_buffer : fb.buffer;
    buf[y * fb.width + x] = color;
}

/*
 * Get pixel color
 */
color_t fb_get_pixel(int x, int y) {
    if (x < 0 || x >= (int)fb.width || y < 0 || y >= (int)fb.height) return 0;
    
    uint32_t *buf = back_buffer ? back_buffer : fb.buffer;
    return buf[y * fb.width + x];
}

/*
 * Draw line using Bresenham's algorithm
 */
void fb_draw_line(int x1, int y1, int x2, int y2, color_t color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int sx = dx > 0 ? 1 : -1;
    int sy = dy > 0 ? 1 : -1;
    
    dx = dx > 0 ? dx : -dx;
    dy = dy > 0 ? dy : -dy;
    
    int err = (dx > dy ? dx : -dy) / 2;
    
    while (1) {
        fb_put_pixel(x1, y1, color);
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 < dy) { err += dx; y1 += sy; }
    }
}

/*
 * Draw rectangle outline
 */
void fb_draw_rect(int x, int y, int w, int h, color_t color) {
    fb_draw_line(x, y, x + w - 1, y, color);
    fb_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    fb_draw_line(x, y, x, y + h - 1, color);
    fb_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

/*
 * Fill rectangle
 */
void fb_fill_rect(int x, int y, int w, int h, color_t color) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            fb_put_pixel(px, py, color);
        }
    }
}

/*
 * Draw circle outline
 */
void fb_draw_circle(int cx, int cy, int r, color_t color) {
    int x = r;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        fb_put_pixel(cx + x, cy + y, color);
        fb_put_pixel(cx + y, cy + x, color);
        fb_put_pixel(cx - y, cy + x, color);
        fb_put_pixel(cx - x, cy + y, color);
        fb_put_pixel(cx - x, cy - y, color);
        fb_put_pixel(cx - y, cy - x, color);
        fb_put_pixel(cx + y, cy - x, color);
        fb_put_pixel(cx + x, cy - y, color);
        
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

/*
 * Fill circle
 */
void fb_fill_circle(int cx, int cy, int r, color_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                fb_put_pixel(cx + x, cy + y, color);
            }
        }
    }
}

/*
 * Draw rounded rectangle
 */
void fb_draw_rounded_rect(int x, int y, int w, int h, int r, color_t color) {
    /* Corners */
    int x1 = x + r, y1 = y + r;
    int x2 = x + w - r - 1, y2 = y + h - r - 1;
    
    /* Top and bottom lines */
    fb_draw_line(x1, y, x2, y, color);
    fb_draw_line(x1, y + h - 1, x2, y + h - 1, color);
    
    /* Left and right lines */
    fb_draw_line(x, y1, x, y2, color);
    fb_draw_line(x + w - 1, y1, x + w - 1, y2, color);
    
    /* Corner arcs */
    int px = r, py = 0, err = 0;
    while (px >= py) {
        fb_put_pixel(x2 + px, y1 - py, color);
        fb_put_pixel(x2 + py, y1 - px, color);
        fb_put_pixel(x1 - py, y1 - px, color);
        fb_put_pixel(x1 - px, y1 - py, color);
        fb_put_pixel(x1 - px, y2 + py, color);
        fb_put_pixel(x1 - py, y2 + px, color);
        fb_put_pixel(x2 + py, y2 + px, color);
        fb_put_pixel(x2 + px, y2 + py, color);
        
        py++;
        if (err <= 0) err += 2 * py + 1;
        if (err > 0) { px--; err -= 2 * px + 1; }
    }
}

/*
 * Fill rounded rectangle
 */
void fb_fill_rounded_rect(int x, int y, int w, int h, int r, color_t color) {
    /* Main body */
    fb_fill_rect(x + r, y, w - 2 * r, h, color);
    fb_fill_rect(x, y + r, r, h - 2 * r, color);
    fb_fill_rect(x + w - r, y + r, r, h - 2 * r, color);
    
    /* Corner circles */
    fb_fill_circle(x + r, y + r, r, color);
    fb_fill_circle(x + w - r - 1, y + r, r, color);
    fb_fill_circle(x + r, y + h - r - 1, r, color);
    fb_fill_circle(x + w - r - 1, y + h - r - 1, r, color);
}

/*
 * Fill vertical gradient
 */
void fb_fill_gradient_v(int x, int y, int w, int h, color_t c1, color_t c2) {
    for (int py = 0; py < h; py++) {
        uint8_t r = GET_R(c1) + (GET_R(c2) - GET_R(c1)) * py / h;
        uint8_t g = GET_G(c1) + (GET_G(c2) - GET_G(c1)) * py / h;
        uint8_t b = GET_B(c1) + (GET_B(c2) - GET_B(c1)) * py / h;
        color_t color = RGB(r, g, b);
        
        for (int px = 0; px < w; px++) {
            fb_put_pixel(x + px, y + py, color);
        }
    }
}

/*
 * Fill horizontal gradient
 */
void fb_fill_gradient_h(int x, int y, int w, int h, color_t c1, color_t c2) {
    for (int px = 0; px < w; px++) {
        uint8_t r = GET_R(c1) + (GET_R(c2) - GET_R(c1)) * px / w;
        uint8_t g = GET_G(c1) + (GET_G(c2) - GET_G(c1)) * px / w;
        uint8_t b = GET_B(c1) + (GET_B(c2) - GET_B(c1)) * px / w;
        color_t color = RGB(r, g, b);
        
        for (int py = 0; py < h; py++) {
            fb_put_pixel(x + px, y + py, color);
        }
    }
}

/*
 * Draw single character
 */
void fb_draw_char(int x, int y, char c, color_t fg, color_t bg) {
    const uint8_t *glyph = font_8x16[(uint8_t)c];
    
    for (int py = 0; py < 16; py++) {
        uint8_t row = glyph[py];
        for (int px = 0; px < 8; px++) {
            if (row & (0x80 >> px)) {
                fb_put_pixel(x + px, y + py, fg);
            } else if (bg != COLOR_TRANSPARENT) {
                fb_put_pixel(x + px, y + py, bg);
            }
        }
    }
}

/*
 * Draw string
 */
void fb_draw_string(int x, int y, const char *str, color_t fg, color_t bg) {
    int cx = x;
    
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += 16;
        } else {
            fb_draw_char(cx, y, *str, fg, bg);
            cx += 8;
        }
        str++;
    }
}

/*
 * Get string width in pixels
 */
int fb_string_width(const char *str) {
    int width = 0;
    int max_width = 0;
    
    while (*str) {
        if (*str == '\n') {
            if (width > max_width) max_width = width;
            width = 0;
        } else {
            width += 8;
        }
        str++;
    }
    
    return width > max_width ? width : max_width;
}

/*
 * Get character height
 */
int fb_char_height(void) {
    return 16;
}

/*
 * Blit bitmap
 */
void fb_blit(int x, int y, uint32_t *bitmap, int w, int h) {
    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            fb_put_pixel(x + px, y + py, bitmap[py * w + px]);
        }
    }
}

/*
 * Blit bitmap with alpha blending
 */
void fb_blit_alpha(int x, int y, uint32_t *bitmap, int w, int h) {
    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            color_t fg = bitmap[py * w + px];
            if (GET_A(fg) == 0) continue;
            
            color_t bg = fb_get_pixel(x + px, y + py);
            fb_put_pixel(x + px, y + py, blend_colors(fg, bg));
        }
    }
}

/*
 * Swap front and back buffers
 */
void fb_swap_buffers(void) {
    if (back_buffer) {
        memcpy(fb.buffer, back_buffer, fb.size);
    }
}

/*
 * Get back buffer
 */
uint32_t *fb_get_back_buffer(void) {
    return back_buffer ? back_buffer : fb.buffer;
}

/*
 * Set clipping rectangle
 */
void fb_set_clip(int x, int y, int w, int h) {
    clip_rect.x = x;
    clip_rect.y = y;
    clip_rect.width = w;
    clip_rect.height = h;
    clipping_enabled = true;
}

/*
 * Reset clipping to full screen
 */
void fb_reset_clip(void) {
    clip_rect.x = 0;
    clip_rect.y = 0;
    clip_rect.width = fb.width;
    clip_rect.height = fb.height;
    clipping_enabled = false;
}
