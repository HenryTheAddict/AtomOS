/*
 * Javier Image Viewer
 * Supports BMP, basic image viewing
 */

#ifndef _JAVIER_IMAGEVIEW_H
#define _JAVIER_IMAGEVIEW_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"

/* Supported image formats */
typedef enum {
    IMG_FORMAT_UNKNOWN,
    IMG_FORMAT_BMP,
    IMG_FORMAT_PPM,
    IMG_FORMAT_TGA,
    IMG_FORMAT_RAW
} image_format_t;

/* Image structure */
typedef struct {
    uint32_t *pixels;
    int width;
    int height;
    int bpp;
    image_format_t format;
    char filename[256];
} image_t;

/* BMP file header */
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} __attribute__((packed)) bmp_file_header_t;

/* BMP info header */
typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_ppm;
    int32_t y_ppm;
    uint32_t colors_used;
    uint32_t colors_important;
} __attribute__((packed)) bmp_info_header_t;

/* Image viewer application */
typedef struct {
    javier_window_t *window;
    image_t *current_image;
    int scroll_x, scroll_y;
    float zoom;
    bool running;
} imageview_t;

/* Image loading */
image_t *image_load(const char *path);
image_t *image_load_bmp(const char *path);
image_t *image_create(int width, int height);
void image_destroy(image_t *img);

/* Image operations */
void image_resize(image_t *src, image_t *dst);
void image_flip_vertical(image_t *img);
void image_flip_horizontal(image_t *img);
void image_rotate_90(image_t *img);
void image_grayscale(image_t *img);
void image_invert(image_t *img);
void image_brightness(image_t *img, int delta);
void image_contrast(image_t *img, float factor);

/* Image viewer functions */
imageview_t *imageview_create(int x, int y, int width, int height);
void imageview_destroy(imageview_t *iv);
bool imageview_open(imageview_t *iv, const char *path);
void imageview_draw(imageview_t *iv);
void imageview_update(imageview_t *iv);
void imageview_zoom_in(imageview_t *iv);
void imageview_zoom_out(imageview_t *iv);
void imageview_fit_to_window(imageview_t *iv);

#endif /* _JAVIER_IMAGEVIEW_H */
