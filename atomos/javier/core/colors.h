/*
 * Javier Color Management System
 * Advanced color handling and themes
 */

#ifndef _JAVIER_COLORS_H
#define _JAVIER_COLORS_H

#include "../../kernel/include/types.h"

/* Color type */
typedef uint32_t color_t;

/* Color components macros */
#define RGBA(r, g, b, a)    (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define RGB(r, g, b)        RGBA(r, g, b, 255)
#define GET_R(c)            (((c) >> 16) & 0xFF)
#define GET_G(c)            (((c) >> 8) & 0xFF)
#define GET_B(c)            ((c) & 0xFF)
#define GET_A(c)            (((c) >> 24) & 0xFF)

/* HSL color */
typedef struct {
    float h;    /* Hue: 0-360 */
    float s;    /* Saturation: 0-1 */
    float l;    /* Lightness: 0-1 */
} hsl_t;

/* HSV color */
typedef struct {
    float h;    /* Hue: 0-360 */
    float s;    /* Saturation: 0-1 */
    float v;    /* Value: 0-1 */
} hsv_t;

/* Lab color (perceptual) */
typedef struct {
    float l;    /* Lightness: 0-100 */
    float a;    /* Green-Red: -128 to 127 */
    float b;    /* Blue-Yellow: -128 to 127 */
} lab_t;

/* ===== STANDARD COLORS ===== */

/* Basic colors */
#define COLOR_BLACK         RGB(0, 0, 0)
#define COLOR_WHITE         RGB(255, 255, 255)
#define COLOR_RED           RGB(255, 0, 0)
#define COLOR_GREEN         RGB(0, 255, 0)
#define COLOR_BLUE          RGB(0, 0, 255)
#define COLOR_YELLOW        RGB(255, 255, 0)
#define COLOR_CYAN          RGB(0, 255, 255)
#define COLOR_MAGENTA       RGB(255, 0, 255)

/* Grays */
#define COLOR_GRAY_10       RGB(26, 26, 26)
#define COLOR_GRAY_20       RGB(51, 51, 51)
#define COLOR_GRAY_30       RGB(77, 77, 77)
#define COLOR_GRAY_40       RGB(102, 102, 102)
#define COLOR_GRAY_50       RGB(128, 128, 128)
#define COLOR_GRAY_60       RGB(153, 153, 153)
#define COLOR_GRAY_70       RGB(179, 179, 179)
#define COLOR_GRAY_80       RGB(204, 204, 204)
#define COLOR_GRAY_90       RGB(230, 230, 230)

/* Material Design Colors */
#define MD_RED_500          RGB(244, 67, 54)
#define MD_PINK_500         RGB(233, 30, 99)
#define MD_PURPLE_500       RGB(156, 39, 176)
#define MD_DEEP_PURPLE_500  RGB(103, 58, 183)
#define MD_INDIGO_500       RGB(63, 81, 181)
#define MD_BLUE_500         RGB(33, 150, 243)
#define MD_LIGHT_BLUE_500   RGB(3, 169, 244)
#define MD_CYAN_500         RGB(0, 188, 212)
#define MD_TEAL_500         RGB(0, 150, 136)
#define MD_GREEN_500        RGB(76, 175, 80)
#define MD_LIGHT_GREEN_500  RGB(139, 195, 74)
#define MD_LIME_500         RGB(205, 220, 57)
#define MD_YELLOW_500       RGB(255, 235, 59)
#define MD_AMBER_500        RGB(255, 193, 7)
#define MD_ORANGE_500       RGB(255, 152, 0)
#define MD_DEEP_ORANGE_500  RGB(255, 87, 34)
#define MD_BROWN_500        RGB(121, 85, 72)
#define MD_GRAY_500         RGB(158, 158, 158)
#define MD_BLUE_GRAY_500    RGB(96, 125, 139)

/* Javier Theme Colors */
#define JAVIER_PRIMARY      RGB(45, 125, 210)
#define JAVIER_SECONDARY    RGB(100, 180, 140)
#define JAVIER_ACCENT       RGB(255, 165, 0)
#define JAVIER_BACKGROUND   RGB(28, 32, 40)
#define JAVIER_SURFACE      RGB(38, 44, 54)
#define JAVIER_TEXT         RGB(230, 235, 240)
#define JAVIER_TEXT_DIM     RGB(150, 160, 175)
#define JAVIER_SUCCESS      RGB(80, 200, 120)
#define JAVIER_WARNING      RGB(255, 180, 50)
#define JAVIER_ERROR        RGB(230, 70, 80)
#define JAVIER_INFO         RGB(60, 160, 240)

/* ===== COLOR CONVERSION ===== */

/* RGB <-> HSL */
hsl_t color_rgb_to_hsl(color_t rgb);
color_t color_hsl_to_rgb(hsl_t hsl);

/* RGB <-> HSV */
hsv_t color_rgb_to_hsv(color_t rgb);
color_t color_hsv_to_rgb(hsv_t hsv);

/* RGB <-> Lab (approximate) */
lab_t color_rgb_to_lab(color_t rgb);
color_t color_lab_to_rgb(lab_t lab);

/* ===== COLOR MANIPULATION ===== */

/* Blend two colors */
color_t color_blend(color_t c1, color_t c2, float ratio);

/* Alpha blend */
color_t color_alpha_blend(color_t fg, color_t bg);

/* Lighten/darken */
color_t color_lighten(color_t c, float amount);
color_t color_darken(color_t c, float amount);

/* Saturate/desaturate */
color_t color_saturate(color_t c, float amount);
color_t color_desaturate(color_t c, float amount);

/* Adjust hue */
color_t color_adjust_hue(color_t c, float degrees);

/* Invert */
color_t color_invert(color_t c);

/* Grayscale */
color_t color_grayscale(color_t c);

/* Sepia */
color_t color_sepia(color_t c);

/* Contrast (WCAG) */
float color_contrast_ratio(color_t c1, color_t c2);
bool color_has_sufficient_contrast(color_t c1, color_t c2, bool large_text);

/* Luminance */
float color_luminance(color_t c);

/* ===== COLOR PALETTES ===== */

/* Generate complementary color */
color_t color_complementary(color_t c);

/* Generate analogous colors */
void color_analogous(color_t c, color_t *out, int count);

/* Generate triadic colors */
void color_triadic(color_t c, color_t *c2, color_t *c3);

/* Generate split-complementary */
void color_split_complementary(color_t c, color_t *c2, color_t *c3);

/* Generate shade palette */
void color_shades(color_t c, color_t *out, int count);

/* Generate tint palette */
void color_tints(color_t c, color_t *out, int count);

/* ===== GRADIENTS ===== */

typedef enum {
    GRADIENT_LINEAR,
    GRADIENT_RADIAL,
    GRADIENT_CONICAL
} gradient_type_t;

typedef struct {
    color_t color;
    float position;  /* 0.0 to 1.0 */
} gradient_stop_t;

typedef struct {
    gradient_type_t type;
    gradient_stop_t stops[16];
    int stop_count;
    float angle;        /* For linear */
    int center_x;       /* For radial/conical */
    int center_y;
    int radius;         /* For radial */
} gradient_t;

/* Create gradients */
gradient_t *gradient_create_linear(float angle);
gradient_t *gradient_create_radial(int cx, int cy, int radius);
void gradient_add_stop(gradient_t *g, color_t color, float position);
color_t gradient_sample(gradient_t *g, int x, int y, int width, int height);
void gradient_destroy(gradient_t *g);

/* ===== THEMES ===== */

typedef struct {
    char name[32];
    color_t primary;
    color_t secondary;
    color_t accent;
    color_t background;
    color_t surface;
    color_t on_primary;
    color_t on_secondary;
    color_t on_background;
    color_t on_surface;
    color_t error;
    color_t on_error;
    color_t window_bg;
    color_t window_title_bg;
    color_t window_title_fg;
    color_t button_bg;
    color_t button_fg;
    color_t button_hover;
    color_t input_bg;
    color_t input_fg;
    color_t input_border;
    color_t highlight;
    color_t selection;
} theme_t;

/* Built-in themes */
extern const theme_t theme_dark;
extern const theme_t theme_light;
extern const theme_t theme_javier;
extern const theme_t theme_neon;
extern const theme_t theme_nature;

/* Theme management */
void theme_set_current(const theme_t *theme);
const theme_t *theme_get_current(void);
color_t theme_color(const char *name);

#endif /* _JAVIER_COLORS_H */
