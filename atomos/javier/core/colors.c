/*
 * Javier Color Management Implementation
 */

#include "colors.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* Forward declarations for math functions */
static float my_sqrtf(float x);
static float my_powf(float base, float exp);
static float my_sinf(float x);
static float my_cosf(float x);

/* Built-in themes */
const theme_t theme_dark = {
    .name = "Dark",
    .primary = RGB(100, 150, 255),
    .secondary = RGB(120, 200, 150),
    .accent = RGB(255, 180, 50),
    .background = RGB(18, 18, 24),
    .surface = RGB(30, 32, 40),
    .on_primary = RGB(255, 255, 255),
    .on_secondary = RGB(0, 0, 0),
    .on_background = RGB(230, 230, 230),
    .on_surface = RGB(200, 200, 200),
    .error = RGB(240, 80, 80),
    .on_error = RGB(255, 255, 255),
    .window_bg = RGB(25, 28, 35),
    .window_title_bg = RGB(35, 40, 50),
    .window_title_fg = RGB(220, 220, 230),
    .button_bg = RGB(50, 55, 70),
    .button_fg = RGB(220, 220, 230),
    .button_hover = RGB(70, 75, 90),
    .input_bg = RGB(20, 22, 28),
    .input_fg = RGB(220, 220, 230),
    .input_border = RGB(60, 65, 80),
    .highlight = RGB(100, 150, 255),
    .selection = RGBA(100, 150, 255, 100)
};

const theme_t theme_light = {
    .name = "Light",
    .primary = RGB(50, 100, 200),
    .secondary = RGB(80, 160, 120),
    .accent = RGB(230, 140, 20),
    .background = RGB(248, 248, 252),
    .surface = RGB(255, 255, 255),
    .on_primary = RGB(255, 255, 255),
    .on_secondary = RGB(255, 255, 255),
    .on_background = RGB(30, 30, 35),
    .on_surface = RGB(50, 50, 55),
    .error = RGB(200, 50, 50),
    .on_error = RGB(255, 255, 255),
    .window_bg = RGB(245, 245, 250),
    .window_title_bg = RGB(235, 235, 245),
    .window_title_fg = RGB(40, 40, 50),
    .button_bg = RGB(230, 230, 240),
    .button_fg = RGB(40, 40, 50),
    .button_hover = RGB(210, 210, 225),
    .input_bg = RGB(255, 255, 255),
    .input_fg = RGB(30, 30, 35),
    .input_border = RGB(180, 180, 195),
    .highlight = RGB(50, 100, 200),
    .selection = RGBA(50, 100, 200, 80)
};

const theme_t theme_javier = {
    .name = "Javier",
    .primary = JAVIER_PRIMARY,
    .secondary = JAVIER_SECONDARY,
    .accent = JAVIER_ACCENT,
    .background = JAVIER_BACKGROUND,
    .surface = JAVIER_SURFACE,
    .on_primary = RGB(255, 255, 255),
    .on_secondary = RGB(0, 0, 0),
    .on_background = JAVIER_TEXT,
    .on_surface = JAVIER_TEXT_DIM,
    .error = JAVIER_ERROR,
    .on_error = RGB(255, 255, 255),
    .window_bg = RGB(32, 38, 48),
    .window_title_bg = RGB(40, 48, 60),
    .window_title_fg = RGB(220, 225, 235),
    .button_bg = RGB(48, 58, 72),
    .button_fg = RGB(220, 225, 235),
    .button_hover = RGB(58, 72, 90),
    .input_bg = RGB(24, 28, 36),
    .input_fg = RGB(220, 225, 235),
    .input_border = RGB(60, 72, 88),
    .highlight = JAVIER_PRIMARY,
    .selection = RGBA(45, 125, 210, 100)
};

const theme_t theme_neon = {
    .name = "Neon",
    .primary = RGB(255, 0, 255),
    .secondary = RGB(0, 255, 255),
    .accent = RGB(255, 255, 0),
    .background = RGB(10, 10, 20),
    .surface = RGB(20, 20, 35),
    .on_primary = RGB(255, 255, 255),
    .on_secondary = RGB(0, 0, 0),
    .on_background = RGB(255, 255, 255),
    .on_surface = RGB(200, 200, 255),
    .error = RGB(255, 50, 100),
    .on_error = RGB(255, 255, 255),
    .window_bg = RGB(15, 15, 30),
    .window_title_bg = RGB(30, 20, 50),
    .window_title_fg = RGB(255, 200, 255),
    .button_bg = RGB(50, 30, 80),
    .button_fg = RGB(255, 200, 255),
    .button_hover = RGB(80, 40, 120),
    .input_bg = RGB(10, 10, 25),
    .input_fg = RGB(200, 255, 255),
    .input_border = RGB(100, 50, 150),
    .highlight = RGB(255, 0, 255),
    .selection = RGBA(255, 0, 255, 80)
};

const theme_t theme_nature = {
    .name = "Nature",
    .primary = RGB(76, 175, 80),
    .secondary = RGB(139, 195, 74),
    .accent = RGB(255, 193, 7),
    .background = RGB(30, 35, 28),
    .surface = RGB(42, 48, 38),
    .on_primary = RGB(255, 255, 255),
    .on_secondary = RGB(0, 0, 0),
    .on_background = RGB(220, 230, 210),
    .on_surface = RGB(180, 195, 170),
    .error = RGB(230, 74, 25),
    .on_error = RGB(255, 255, 255),
    .window_bg = RGB(35, 42, 32),
    .window_title_bg = RGB(50, 60, 45),
    .window_title_fg = RGB(210, 225, 200),
    .button_bg = RGB(55, 70, 50),
    .button_fg = RGB(210, 225, 200),
    .button_hover = RGB(70, 90, 65),
    .input_bg = RGB(28, 32, 25),
    .input_fg = RGB(210, 225, 200),
    .input_border = RGB(70, 85, 65),
    .highlight = RGB(76, 175, 80),
    .selection = RGBA(76, 175, 80, 80)
};

/* Current theme */
static const theme_t *current_theme = &theme_javier;

/* Helper: clamp float to 0-1 */
static float clampf(float x) {
    if (x < 0) return 0;
    if (x > 1) return 1;
    return x;
}

/* Helper: clamp int to 0-255 */
static int clampi(int x) {
    if (x < 0) return 0;
    if (x > 255) return 255;
    return x;
}

/* Helper: min/max */
static float minf(float a, float b) { return a < b ? a : b; }
static float maxf(float a, float b) { return a > b ? a : b; }

/*
 * RGB to HSL
 */
hsl_t color_rgb_to_hsl(color_t rgb) {
    float r = GET_R(rgb) / 255.0f;
    float g = GET_G(rgb) / 255.0f;
    float b = GET_B(rgb) / 255.0f;
    
    float max = maxf(maxf(r, g), b);
    float min = minf(minf(r, g), b);
    float delta = max - min;
    
    hsl_t hsl;
    hsl.l = (max + min) / 2;
    
    if (delta < 0.001f) {
        hsl.h = 0;
        hsl.s = 0;
    } else {
        hsl.s = hsl.l > 0.5f ? delta / (2 - max - min) : delta / (max + min);
        
        if (max == r) {
            hsl.h = (g - b) / delta + (g < b ? 6 : 0);
        } else if (max == g) {
            hsl.h = (b - r) / delta + 2;
        } else {
            hsl.h = (r - g) / delta + 4;
        }
        hsl.h *= 60;
    }
    
    return hsl;
}

/*
 * HSL to RGB
 */
static float hue_to_rgb(float p, float q, float t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6 * t;
    if (t < 1.0f/2) return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6;
    return p;
}

color_t color_hsl_to_rgb(hsl_t hsl) {
    float r, g, b;
    
    if (hsl.s < 0.001f) {
        r = g = b = hsl.l;
    } else {
        float q = hsl.l < 0.5f ? hsl.l * (1 + hsl.s) : hsl.l + hsl.s - hsl.l * hsl.s;
        float p = 2 * hsl.l - q;
        float h = hsl.h / 360.0f;
        
        r = hue_to_rgb(p, q, h + 1.0f/3);
        g = hue_to_rgb(p, q, h);
        b = hue_to_rgb(p, q, h - 1.0f/3);
    }
    
    return RGB((int)(r * 255), (int)(g * 255), (int)(b * 255));
}

/*
 * RGB to HSV
 */
hsv_t color_rgb_to_hsv(color_t rgb) {
    float r = GET_R(rgb) / 255.0f;
    float g = GET_G(rgb) / 255.0f;
    float b = GET_B(rgb) / 255.0f;
    
    float max = maxf(maxf(r, g), b);
    float min = minf(minf(r, g), b);
    float delta = max - min;
    
    hsv_t hsv;
    hsv.v = max;
    
    if (max < 0.001f) {
        hsv.s = 0;
        hsv.h = 0;
    } else {
        hsv.s = delta / max;
        
        if (delta < 0.001f) {
            hsv.h = 0;
        } else if (max == r) {
            hsv.h = 60 * (g - b) / delta;
            if (hsv.h < 0) hsv.h += 360;
        } else if (max == g) {
            hsv.h = 60 * (2 + (b - r) / delta);
        } else {
            hsv.h = 60 * (4 + (r - g) / delta);
        }
    }
    
    return hsv;
}

/*
 * HSV to RGB
 */
color_t color_hsv_to_rgb(hsv_t hsv) {
    float h = hsv.h;
    float s = hsv.s;
    float v = hsv.v;
    
    float c = v * s;
    float x = c * (1 - ((h / 60) - 2 * (int)(h / 120) > 0 ? 
                        (h / 60) - 2 * (int)(h / 120) : 
                        -(h / 60) + 2 * (int)(h / 120) + 1));
    float m = v - c;
    
    float r, g, b;
    int sector = (int)(h / 60) % 6;
    
    switch (sector) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default: r = c; g = 0; b = x; break;
    }
    
    return RGB((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255));
}

/*
 * Blend colors
 */
color_t color_blend(color_t c1, color_t c2, float ratio) {
    ratio = clampf(ratio);
    float inv = 1 - ratio;
    
    int r = (int)(GET_R(c1) * inv + GET_R(c2) * ratio);
    int g = (int)(GET_G(c1) * inv + GET_G(c2) * ratio);
    int b = (int)(GET_B(c1) * inv + GET_B(c2) * ratio);
    
    return RGB(clampi(r), clampi(g), clampi(b));
}

/*
 * Alpha blend
 */
color_t color_alpha_blend(color_t fg, color_t bg) {
    float a = GET_A(fg) / 255.0f;
    float inv = 1 - a;
    
    int r = (int)(GET_R(fg) * a + GET_R(bg) * inv);
    int g = (int)(GET_G(fg) * a + GET_G(bg) * inv);
    int b = (int)(GET_B(fg) * a + GET_B(bg) * inv);
    
    return RGB(clampi(r), clampi(g), clampi(b));
}

/*
 * Lighten
 */
color_t color_lighten(color_t c, float amount) {
    hsl_t hsl = color_rgb_to_hsl(c);
    hsl.l = clampf(hsl.l + amount);
    return color_hsl_to_rgb(hsl);
}

/*
 * Darken
 */
color_t color_darken(color_t c, float amount) {
    return color_lighten(c, -amount);
}

/*
 * Saturate
 */
color_t color_saturate(color_t c, float amount) {
    hsl_t hsl = color_rgb_to_hsl(c);
    hsl.s = clampf(hsl.s + amount);
    return color_hsl_to_rgb(hsl);
}

/*
 * Desaturate
 */
color_t color_desaturate(color_t c, float amount) {
    return color_saturate(c, -amount);
}

/*
 * Adjust hue
 */
color_t color_adjust_hue(color_t c, float degrees) {
    hsl_t hsl = color_rgb_to_hsl(c);
    hsl.h = hsl.h + degrees;
    while (hsl.h < 0) hsl.h += 360;
    while (hsl.h >= 360) hsl.h -= 360;
    return color_hsl_to_rgb(hsl);
}

/*
 * Invert
 */
color_t color_invert(color_t c) {
    return RGB(255 - GET_R(c), 255 - GET_G(c), 255 - GET_B(c));
}

/*
 * Grayscale
 */
color_t color_grayscale(color_t c) {
    /* ITU-R BT.709 coefficients */
    int gray = (GET_R(c) * 2126 + GET_G(c) * 7152 + GET_B(c) * 722) / 10000;
    return RGB(gray, gray, gray);
}

/*
 * Sepia
 */
color_t color_sepia(color_t c) {
    int r = GET_R(c);
    int g = GET_G(c);
    int b = GET_B(c);
    
    int tr = clampi((r * 393 + g * 769 + b * 189) / 1000);
    int tg = clampi((r * 349 + g * 686 + b * 168) / 1000);
    int tb = clampi((r * 272 + g * 534 + b * 131) / 1000);
    
    return RGB(tr, tg, tb);
}

/*
 * Luminance (relative)
 */
float color_luminance(color_t c) {
    float r = GET_R(c) / 255.0f;
    float g = GET_G(c) / 255.0f;
    float b = GET_B(c) / 255.0f;
    
    /* sRGB to linear (simplified for kernel) */
    r = r <= 0.03928f ? r / 12.92f : my_powf((r + 0.055f) / 1.055f, 2.4f);
    g = g <= 0.03928f ? g / 12.92f : my_powf((g + 0.055f) / 1.055f, 2.4f);
    b = b <= 0.03928f ? b / 12.92f : my_powf((b + 0.055f) / 1.055f, 2.4f);
    
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

/*
 * Contrast ratio (WCAG)
 */
float color_contrast_ratio(color_t c1, color_t c2) {
    float l1 = color_luminance(c1);
    float l2 = color_luminance(c2);
    
    if (l2 > l1) {
        float tmp = l1;
        l1 = l2;
        l2 = tmp;
    }
    
    return (l1 + 0.05f) / (l2 + 0.05f);
}

/*
 * Check sufficient contrast
 */
bool color_has_sufficient_contrast(color_t c1, color_t c2, bool large_text) {
    float ratio = color_contrast_ratio(c1, c2);
    return large_text ? (ratio >= 3.0f) : (ratio >= 4.5f);
}

/*
 * Complementary color
 */
color_t color_complementary(color_t c) {
    return color_adjust_hue(c, 180);
}

/*
 * Generate analogous colors
 */
void color_analogous(color_t c, color_t *out, int count) {
    float step = 30.0f;
    float start = -(count - 1) * step / 2;
    
    for (int i = 0; i < count; i++) {
        out[i] = color_adjust_hue(c, start + i * step);
    }
}

/*
 * Generate triadic colors
 */
void color_triadic(color_t c, color_t *c2, color_t *c3) {
    *c2 = color_adjust_hue(c, 120);
    *c3 = color_adjust_hue(c, 240);
}

/*
 * Generate split-complementary
 */
void color_split_complementary(color_t c, color_t *c2, color_t *c3) {
    *c2 = color_adjust_hue(c, 150);
    *c3 = color_adjust_hue(c, 210);
}

/*
 * Generate shades
 */
void color_shades(color_t c, color_t *out, int count) {
    for (int i = 0; i < count; i++) {
        float factor = (float)i / (count - 1);
        out[i] = color_darken(c, factor * 0.7f);
    }
}

/*
 * Generate tints
 */
void color_tints(color_t c, color_t *out, int count) {
    for (int i = 0; i < count; i++) {
        float factor = (float)i / (count - 1);
        out[i] = color_lighten(c, factor * 0.7f);
    }
}

/*
 * Create linear gradient
 */
gradient_t *gradient_create_linear(float angle) {
    gradient_t *g = (gradient_t *)kcalloc(1, sizeof(gradient_t));
    if (g) {
        g->type = GRADIENT_LINEAR;
        g->angle = angle;
    }
    return g;
}

/*
 * Create radial gradient
 */
gradient_t *gradient_create_radial(int cx, int cy, int radius) {
    gradient_t *g = (gradient_t *)kcalloc(1, sizeof(gradient_t));
    if (g) {
        g->type = GRADIENT_RADIAL;
        g->center_x = cx;
        g->center_y = cy;
        g->radius = radius;
    }
    return g;
}

/*
 * Add gradient stop
 */
void gradient_add_stop(gradient_t *g, color_t color, float position) {
    if (g && g->stop_count < 16) {
        g->stops[g->stop_count].color = color;
        g->stops[g->stop_count].position = clampf(position);
        g->stop_count++;
    }
}

/*
 * Sample gradient
 */
color_t gradient_sample(gradient_t *g, int x, int y, int width, int height) {
    if (!g || g->stop_count < 2) return COLOR_BLACK;
    
    float t = 0;
    
    switch (g->type) {
        case GRADIENT_LINEAR: {
            /* Calculate position along gradient angle */
            float angle_rad = g->angle * 3.14159f / 180.0f;
            float dx = my_cosf(angle_rad);
            float dy = my_sinf(angle_rad);
            float nx = (float)x / width - 0.5f;
            float ny = (float)y / height - 0.5f;
            t = (nx * dx + ny * dy) + 0.5f;
            break;
        }
        case GRADIENT_RADIAL: {
            float rdx = (float)(x - g->center_x);
            float rdy = (float)(y - g->center_y);
            float dist = my_sqrtf(rdx * rdx + rdy * rdy);
            t = dist / g->radius;
            break;
        }
        default:
            t = (float)x / width;
            break;
    }
    
    t = clampf(t);
    
    /* Find stops */
    int i;
    for (i = 1; i < g->stop_count; i++) {
        if (t < g->stops[i].position) break;
    }
    
    if (i >= g->stop_count) {
        return g->stops[g->stop_count - 1].color;
    }
    
    float range = g->stops[i].position - g->stops[i-1].position;
    float ratio = range > 0 ? (t - g->stops[i-1].position) / range : 0;
    
    return color_blend(g->stops[i-1].color, g->stops[i].color, ratio);
}

/*
 * Destroy gradient
 */
void gradient_destroy(gradient_t *g) {
    if (g) kfree(g);
}

/*
 * Set current theme
 */
void theme_set_current(const theme_t *theme) {
    current_theme = theme ? theme : &theme_javier;
}

/*
 * Get current theme
 */
const theme_t *theme_get_current(void) {
    return current_theme;
}

/*
 * Get theme color by name
 */
color_t theme_color(const char *name) {
    if (!name || !current_theme) return COLOR_BLACK;
    
    if (strcmp(name, "primary") == 0) return current_theme->primary;
    if (strcmp(name, "secondary") == 0) return current_theme->secondary;
    if (strcmp(name, "accent") == 0) return current_theme->accent;
    if (strcmp(name, "background") == 0) return current_theme->background;
    if (strcmp(name, "surface") == 0) return current_theme->surface;
    if (strcmp(name, "error") == 0) return current_theme->error;
    if (strcmp(name, "highlight") == 0) return current_theme->highlight;
    
    return COLOR_BLACK;
}

/* Simple sqrt implementation for color calculations */
static float my_sqrtf(float x) {
    if (x <= 0) return 0;
    float guess = x / 2;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2;
    }
    return guess;
}

/* Simple pow implementation for color calculations */
static float my_powf(float base, float exp) {
    /* Fast approximation for gamma correction */
    if (exp >= 2.0f && exp <= 3.0f) {
        return base * base * my_sqrtf(base);
    }
    return base * base;  /* Fallback */
}

static float my_cosf(float x) {
    /* Taylor series approximation */
    while (x > 6.283185f) x -= 6.283185f;
    while (x < 0) x += 6.283185f;
    float x2 = x * x;
    return 1.0f - x2/2.0f + x2*x2/24.0f - x2*x2*x2/720.0f;
}

static float my_sinf(float x) {
    return my_cosf(x - 1.5707963f);
}
