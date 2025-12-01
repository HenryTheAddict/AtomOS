/*
 * Javier Image Viewer Implementation
 */

#include "imageview.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/fs/vfs.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"

/*
 * Create empty image
 */
image_t *image_create(int width, int height) {
    image_t *img = (image_t *)kcalloc(1, sizeof(image_t));
    if (!img) return NULL;
    
    img->width = width;
    img->height = height;
    img->bpp = 32;
    img->pixels = (uint32_t *)kcalloc(width * height, sizeof(uint32_t));
    
    if (!img->pixels) {
        kfree(img);
        return NULL;
    }
    
    return img;
}

/*
 * Destroy image
 */
void image_destroy(image_t *img) {
    if (img) {
        if (img->pixels) kfree(img->pixels);
        kfree(img);
    }
}

/*
 * Load BMP image
 */
image_t *image_load_bmp(const char *path) {
    fd_t fd = vfs_open(path, O_RDONLY, 0);
    if (fd < 0) {
        kerror("ImageView: Cannot open %s\n", path);
        return NULL;
    }
    
    /* Read file header */
    bmp_file_header_t file_header;
    if (vfs_read(fd, &file_header, sizeof(file_header)) != sizeof(file_header)) {
        vfs_close(fd);
        return NULL;
    }
    
    /* Check magic */
    if (file_header.type != 0x4D42) {  /* "BM" */
        kerror("ImageView: Not a BMP file\n");
        vfs_close(fd);
        return NULL;
    }
    
    /* Read info header */
    bmp_info_header_t info_header;
    if (vfs_read(fd, &info_header, sizeof(info_header)) != sizeof(info_header)) {
        vfs_close(fd);
        return NULL;
    }
    
    /* Only support 24 and 32 bpp for now */
    if (info_header.bpp != 24 && info_header.bpp != 32) {
        kerror("ImageView: Unsupported BPP %d\n", info_header.bpp);
        vfs_close(fd);
        return NULL;
    }
    
    /* Create image */
    int width = info_header.width;
    int height = info_header.height < 0 ? -info_header.height : info_header.height;
    bool flip = info_header.height > 0;  /* BMP is bottom-up by default */
    
    image_t *img = image_create(width, height);
    if (!img) {
        vfs_close(fd);
        return NULL;
    }
    
    img->format = IMG_FORMAT_BMP;
    strncpy(img->filename, path, sizeof(img->filename) - 1);
    
    /* Seek to pixel data */
    vfs_seek(fd, file_header.offset, SEEK_SET);
    
    /* Calculate row size (BMP rows are 4-byte aligned) */
    int bytes_per_pixel = info_header.bpp / 8;
    int row_size = ((width * bytes_per_pixel) + 3) & ~3;
    
    /* Read pixel data */
    uint8_t *row = (uint8_t *)kmalloc(row_size);
    if (!row) {
        image_destroy(img);
        vfs_close(fd);
        return NULL;
    }
    
    for (int y = 0; y < height; y++) {
        int target_y = flip ? (height - 1 - y) : y;
        
        if (vfs_read(fd, row, row_size) != row_size) {
            break;
        }
        
        for (int x = 0; x < width; x++) {
            uint8_t *pixel = &row[x * bytes_per_pixel];
            uint8_t b = pixel[0];
            uint8_t g = pixel[1];
            uint8_t r = pixel[2];
            uint8_t a = (bytes_per_pixel == 4) ? pixel[3] : 255;
            
            img->pixels[target_y * width + x] = RGB(r, g, b);
            (void)a;  /* Alpha not used yet */
        }
    }
    
    kfree(row);
    vfs_close(fd);
    
    kprintf("ImageView: Loaded %s (%dx%d)\n", path, width, height);
    return img;
}

/*
 * Load image (auto-detect format)
 */
image_t *image_load(const char *path) {
    /* Check extension */
    const char *ext = strrchr(path, '.');
    if (ext) {
        if (strcmp(ext, ".bmp") == 0 || strcmp(ext, ".BMP") == 0) {
            return image_load_bmp(path);
        }
    }
    
    /* Try BMP by default */
    return image_load_bmp(path);
}

/*
 * Flip image vertically
 */
void image_flip_vertical(image_t *img) {
    if (!img || !img->pixels) return;
    
    uint32_t *temp = (uint32_t *)kmalloc(img->width * sizeof(uint32_t));
    if (!temp) return;
    
    for (int y = 0; y < img->height / 2; y++) {
        int y2 = img->height - 1 - y;
        memcpy(temp, &img->pixels[y * img->width], img->width * sizeof(uint32_t));
        memcpy(&img->pixels[y * img->width], &img->pixels[y2 * img->width], 
               img->width * sizeof(uint32_t));
        memcpy(&img->pixels[y2 * img->width], temp, img->width * sizeof(uint32_t));
    }
    
    kfree(temp);
}

/*
 * Flip image horizontally
 */
void image_flip_horizontal(image_t *img) {
    if (!img || !img->pixels) return;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width / 2; x++) {
            int x2 = img->width - 1 - x;
            int idx1 = y * img->width + x;
            int idx2 = y * img->width + x2;
            
            uint32_t temp = img->pixels[idx1];
            img->pixels[idx1] = img->pixels[idx2];
            img->pixels[idx2] = temp;
        }
    }
}

/*
 * Convert to grayscale
 */
void image_grayscale(image_t *img) {
    if (!img || !img->pixels) return;
    
    for (int i = 0; i < img->width * img->height; i++) {
        uint32_t pixel = img->pixels[i];
        int r = GET_R(pixel);
        int g = GET_G(pixel);
        int b = GET_B(pixel);
        int gray = (r * 299 + g * 587 + b * 114) / 1000;
        img->pixels[i] = RGB(gray, gray, gray);
    }
}

/*
 * Invert colors
 */
void image_invert(image_t *img) {
    if (!img || !img->pixels) return;
    
    for (int i = 0; i < img->width * img->height; i++) {
        uint32_t pixel = img->pixels[i];
        int r = 255 - GET_R(pixel);
        int g = 255 - GET_G(pixel);
        int b = 255 - GET_B(pixel);
        img->pixels[i] = RGB(r, g, b);
    }
}

/*
 * Adjust brightness
 */
void image_brightness(image_t *img, int delta) {
    if (!img || !img->pixels) return;
    
    for (int i = 0; i < img->width * img->height; i++) {
        uint32_t pixel = img->pixels[i];
        int r = GET_R(pixel) + delta;
        int g = GET_G(pixel) + delta;
        int b = GET_B(pixel) + delta;
        
        if (r < 0) r = 0;
        if (r > 255) r = 255;
        if (g < 0) g = 0;
        if (g > 255) g = 255;
        if (b < 0) b = 0;
        if (b > 255) b = 255;
        
        img->pixels[i] = RGB(r, g, b);
    }
}

/*
 * Create image viewer
 */
imageview_t *imageview_create(int x, int y, int width, int height) {
    imageview_t *iv = (imageview_t *)kcalloc(1, sizeof(imageview_t));
    if (!iv) return NULL;
    
    iv->window = javier_window_create("Image Viewer", x, y, width, height,
                                       WIN_DEFAULT | WIN_RESIZABLE);
    if (!iv->window) {
        kfree(iv);
        return NULL;
    }
    
    iv->zoom = 1.0f;
    iv->running = true;
    
    return iv;
}

/*
 * Destroy image viewer
 */
void imageview_destroy(imageview_t *iv) {
    if (iv) {
        if (iv->window) javier_window_destroy(iv->window);
        if (iv->current_image) image_destroy(iv->current_image);
        kfree(iv);
    }
}

/*
 * Open image
 */
bool imageview_open(imageview_t *iv, const char *path) {
    if (!iv) return false;
    
    if (iv->current_image) {
        image_destroy(iv->current_image);
    }
    
    iv->current_image = image_load(path);
    if (!iv->current_image) {
        return false;
    }
    
    iv->scroll_x = 0;
    iv->scroll_y = 0;
    iv->zoom = 1.0f;
    
    /* Update window title */
    char title[256];
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;
    snprintf(title, sizeof(title), "Image Viewer - %s (%dx%d)",
             filename, iv->current_image->width, iv->current_image->height);
    /* javier_window_set_title(iv->window, title); */
    
    return true;
}

/*
 * Draw image viewer
 */
void imageview_draw(imageview_t *iv) {
    if (!iv || !iv->window) return;
    
    /* Get client area */
    int cx = iv->window->bounds.x + iv->window->client.x;
    int cy = iv->window->bounds.y + iv->window->client.y;
    int cw = iv->window->client.width;
    int ch = iv->window->client.height;
    
    /* Draw background */
    fb_fill_rect(cx, cy, cw, ch, RGB(40, 40, 40));
    
    /* Draw checkerboard pattern for transparency */
    for (int y = 0; y < ch; y += 16) {
        for (int x = 0; x < cw; x += 16) {
            int dark = ((x / 16) + (y / 16)) % 2;
            fb_fill_rect(cx + x, cy + y, 16, 16, 
                        dark ? RGB(50, 50, 50) : RGB(60, 60, 60));
        }
    }
    
    if (!iv->current_image) {
        /* No image loaded - show message */
        fb_draw_string(cx + cw/2 - 80, cy + ch/2, 
                       "No image loaded", RGB(150, 150, 150), RGB(40, 40, 40));
        fb_draw_string(cx + cw/2 - 100, cy + ch/2 + 20,
                       "Press O to open a file", RGB(100, 100, 100), RGB(40, 40, 40));
        return;
    }
    
    /* Calculate displayed size */
    int disp_w = (int)(iv->current_image->width * iv->zoom);
    int disp_h = (int)(iv->current_image->height * iv->zoom);
    
    /* Center image */
    int img_x = cx + (cw - disp_w) / 2 - iv->scroll_x;
    int img_y = cy + (ch - disp_h) / 2 - iv->scroll_y;
    
    /* Draw image (simple nearest-neighbor scaling) */
    for (int y = 0; y < disp_h; y++) {
        int dy = img_y + y;
        if (dy < cy || dy >= cy + ch) continue;
        
        int src_y = (int)(y / iv->zoom);
        if (src_y >= iv->current_image->height) continue;
        
        for (int x = 0; x < disp_w; x++) {
            int dx = img_x + x;
            if (dx < cx || dx >= cx + cw) continue;
            
            int src_x = (int)(x / iv->zoom);
            if (src_x >= iv->current_image->width) continue;
            
            uint32_t pixel = iv->current_image->pixels[
                src_y * iv->current_image->width + src_x];
            fb_put_pixel(dx, dy, pixel);
        }
    }
    
    /* Draw info bar at bottom */
    fb_fill_rect(cx, cy + ch - 24, cw, 24, RGB(30, 30, 30));
    
    char info[128];
    snprintf(info, sizeof(info), "%dx%d  Zoom: %d%%",
             iv->current_image->width, iv->current_image->height,
             (int)(iv->zoom * 100));
    fb_draw_string(cx + 10, cy + ch - 18, info, RGB(200, 200, 200), RGB(30, 30, 30));
}

/*
 * Update image viewer
 */
void imageview_update(imageview_t *iv) {
    if (!iv || !iv->running) return;
    
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            switch (event.scancode) {
                case KEY_ESC:
                    iv->running = false;
                    break;
                case KEY_UP:
                    iv->scroll_y -= 20;
                    break;
                case KEY_DOWN:
                    iv->scroll_y += 20;
                    break;
                case KEY_LEFT:
                    iv->scroll_x -= 20;
                    break;
                case KEY_RIGHT:
                    iv->scroll_x += 20;
                    break;
                default:
                    /* Handle ASCII keys */
                    if (event.ascii == '+' || event.ascii == '=') {
                        imageview_zoom_in(iv);
                    } else if (event.ascii == '-') {
                        imageview_zoom_out(iv);
                    } else if (event.ascii == '0') {
                        iv->zoom = 1.0f;
                        iv->scroll_x = 0;
                        iv->scroll_y = 0;
                    } else if (event.ascii == 'f' || event.ascii == 'F') {
                        imageview_fit_to_window(iv);
                    } else if (event.ascii == 'g' || event.ascii == 'G') {
                        if (iv->current_image) image_grayscale(iv->current_image);
                    } else if (event.ascii == 'i' || event.ascii == 'I') {
                        if (iv->current_image) image_invert(iv->current_image);
                    }
                    break;
            }
        }
    }
}

/*
 * Zoom in
 */
void imageview_zoom_in(imageview_t *iv) {
    if (!iv) return;
    iv->zoom *= 1.25f;
    if (iv->zoom > 10.0f) iv->zoom = 10.0f;
}

/*
 * Zoom out
 */
void imageview_zoom_out(imageview_t *iv) {
    if (!iv) return;
    iv->zoom /= 1.25f;
    if (iv->zoom < 0.1f) iv->zoom = 0.1f;
}

/*
 * Fit image to window
 */
void imageview_fit_to_window(imageview_t *iv) {
    if (!iv || !iv->current_image || !iv->window) return;
    
    int cw = iv->window->client.width;
    int ch = iv->window->client.height - 24;  /* Subtract info bar */
    
    float zoom_w = (float)cw / iv->current_image->width;
    float zoom_h = (float)ch / iv->current_image->height;
    
    iv->zoom = (zoom_w < zoom_h) ? zoom_w : zoom_h;
    iv->scroll_x = 0;
    iv->scroll_y = 0;
}
