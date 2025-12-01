/*
 * JavierApp Application Format Implementation
 * Loader and runtime for .javierapp files
 */

#include "javierapp.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/mm/smart_ram.h"
#include "../../kernel/fs/vfs.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/timer.h"

/* Compatibility defines */
#ifndef KEY_ESCAPE
#define KEY_ESCAPE KEY_ESC
#endif

/* Global API table */
static javierapp_api_t g_api;

/* CRC32 lookup table */
static uint32_t crc32_table[256];
static bool crc32_initialized = false;

static void init_crc32_table(void) {
    if (crc32_initialized) return;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_initialized = true;
}

/*
 * Calculate checksum
 */
uint32_t javierapp_checksum(const void *data, size_t size) {
    init_crc32_table();
    
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; i++) {
        crc = crc32_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

/*
 * Validate application file
 */
bool javierapp_validate(const void *data, size_t size) {
    if (size < sizeof(javierapp_header_t)) {
        return false;
    }
    
    const javierapp_header_t *header = (const javierapp_header_t *)data;
    
    if (header->magic != JAVIERAPP_MAGIC) {
        kerror("JavierApp: Invalid magic number\n");
        return false;
    }
    
    if (header->version > JAVIERAPP_VERSION) {
        kerror("JavierApp: Unsupported version %d\n", header->version);
        return false;
    }
    
    if (header->total_size != size) {
        kerror("JavierApp: Size mismatch\n");
        return false;
    }
    
    /* Checksum verification would go here */
    /* For now, basic validation is sufficient */
    
    return true;
}

/*
 * Load application from file
 */
javierapp_instance_t *javierapp_load(const char *path) {
    /* Open file */
    fd_t fd = vfs_open(path, O_RDONLY, 0);
    if (fd < 0) {
        kerror("JavierApp: Cannot open %s\n", path);
        return NULL;
    }
    
    /* Get file size */
    struct stat st;
    if (vfs_stat(path, &st) < 0) {
        vfs_close(fd);
        return NULL;
    }
    
    /* Allocate buffer for file */
    void *file_data = smart_alloc(st.st_size, ALLOC_NORMAL);
    if (!file_data) {
        vfs_close(fd);
        return NULL;
    }
    
    /* Read entire file */
    ssize_t bytes = vfs_read(fd, file_data, st.st_size);
    vfs_close(fd);
    
    if (bytes != (ssize_t)st.st_size) {
        smart_free(file_data);
        return NULL;
    }
    
    /* Validate */
    if (!javierapp_validate(file_data, st.st_size)) {
        smart_free(file_data);
        return NULL;
    }
    
    /* Create instance */
    javierapp_instance_t *app = smart_alloc(sizeof(javierapp_instance_t), ALLOC_ZERO);
    if (!app) {
        smart_free(file_data);
        return NULL;
    }
    
    /* Copy header */
    memcpy(&app->header, file_data, sizeof(javierapp_header_t));
    
    /* Load metadata if present */
    if (app->header.data_offset > sizeof(javierapp_header_t)) {
        size_t meta_size = app->header.data_offset - sizeof(javierapp_header_t);
        if (meta_size >= sizeof(javierapp_metadata_t)) {
            memcpy(&app->metadata, 
                   (uint8_t *)file_data + sizeof(javierapp_header_t),
                   sizeof(javierapp_metadata_t));
        }
    }
    
    /* Allocate and load code section */
    if (app->header.code_size > 0) {
        app->code_base = smart_alloc_region(app->header.code_size, MEM_REGION_USER);
        if (app->code_base) {
            memcpy(app->code_base,
                   (uint8_t *)file_data + app->header.code_offset,
                   app->header.code_size);
        }
    }
    
    /* Allocate and load data section */
    if (app->header.data_size > 0) {
        app->data_base = smart_alloc_region(app->header.data_size, MEM_REGION_USER);
        if (app->data_base) {
            memcpy(app->data_base,
                   (uint8_t *)file_data + app->header.data_offset,
                   app->header.data_size);
        }
    }
    
    /* Allocate stack */
    app->stack_size = 64 * 1024;  /* 64KB stack */
    app->stack_base = smart_alloc_region(app->stack_size, MEM_REGION_USER);
    
    /* Create main window if GUI app */
    if (app->header.flags & APP_FLAG_GUI) {
        int width = 800, height = 600;
        if (app->header.flags & APP_FLAG_FULLSCREEN) {
            /* Get screen size */
            framebuffer_t *fb = fb_get_info();
            if (fb) {
                width = fb->width;
                height = fb->height;
            }
        }
        
        app->main_window = javier_window_create(
            app->metadata.name[0] ? app->metadata.name : "Application",
            100, 100, width, height,
            WIN_DEFAULT | WIN_RESIZABLE
        );
    }
    
    app->running = true;
    app->exit_code = 0;
    
    smart_free(file_data);
    
    kprintf("JavierApp: Loaded '%s'\n", 
            app->metadata.name[0] ? app->metadata.name : path);
    
    return app;
}

/* API implementation functions */
static void api_draw_pixel(int x, int y, uint32_t color) {
    fb_put_pixel(x, y, color);
}

static void api_draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    fb_draw_line(x1, y1, x2, y2, color);
}

static void api_draw_rect(int x, int y, int w, int h, uint32_t color) {
    fb_fill_rect(x, y, w, h, color);
}

static void api_draw_text(int x, int y, const char *text, uint32_t color) {
    fb_draw_string(x, y, text, color, 0);
}

static void api_present(void) {
    fb_swap_buffers();
}

static int api_get_key(void) {
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            return event.ascii ? event.ascii : event.scancode;
        }
    }
    return 0;
}

static int api_get_mouse(int *x, int *y) {
    /* Basic mouse stub */
    if (x) *x = 0;
    if (y) *y = 0;
    return 0;
}

static void *api_alloc(size_t size) {
    return smart_alloc(size, ALLOC_NORMAL);
}

static void api_free(void *ptr) {
    smart_free(ptr);
}

static int api_open_file(const char *path, int mode) {
    return vfs_open(path, mode, 0);
}

static int api_read_file(int fd, void *buf, size_t size) {
    return vfs_read(fd, buf, size);
}

static int api_write_file(int fd, const void *buf, size_t size) {
    return vfs_write(fd, buf, size);
}

static void api_close_file(int fd) {
    vfs_close(fd);
}

static void api_print(const char *str) {
    kprintf("%s", str);
}

static uint64_t api_get_time(void) {
    return timer_get_uptime() * 1000;  /* Return milliseconds */
}

static void api_sleep(uint32_t ms) {
    timer_sleep_ms(ms);
}

static int g_exit_code = 0;

static void api_exit(int code) {
    g_exit_code = code;
}

/*
 * Get API table
 */
javierapp_api_t *javierapp_get_api(void) {
    static bool initialized = false;
    
    if (!initialized) {
        g_api.draw_pixel = api_draw_pixel;
        g_api.draw_line = api_draw_line;
        g_api.draw_rect = api_draw_rect;
        g_api.draw_text = api_draw_text;
        g_api.present = api_present;
        g_api.get_key = api_get_key;
        g_api.get_mouse = api_get_mouse;
        g_api.alloc = api_alloc;
        g_api.free = api_free;
        g_api.open_file = api_open_file;
        g_api.read_file = api_read_file;
        g_api.write_file = api_write_file;
        g_api.close_file = api_close_file;
        g_api.print = api_print;
        g_api.get_time = api_get_time;
        g_api.sleep = api_sleep;
        g_api.exit = api_exit;
        initialized = true;
    }
    
    return &g_api;
}

/*
 * Run application
 */
int javierapp_run(javierapp_instance_t *app) {
    if (!app || !app->running) return -1;
    
    kprintf("JavierApp: Running '%s'\n", app->metadata.name);
    
    /* For now, we'll just simulate running */
    /* In a real implementation, this would set up the execution context
       and jump to the entry point */
    
    /* Simulate app running */
    if (app->header.flags & APP_FLAG_GUI && app->main_window) {
        javier_window_show(app->main_window);
        
        /* Basic event loop */
        while (app->running) {
            /* Process events */
            if (keyboard_has_key()) {
                key_event_t event = keyboard_get_key();
                if (event.pressed && event.scancode == KEY_ESC) {
                    app->running = false;
                }
            }
            
            /* Let the scheduler run */
            timer_sleep_ms(16);  /* ~60 FPS */
        }
    }
    
    return app->exit_code;
}

/*
 * Terminate application
 */
void javierapp_terminate(javierapp_instance_t *app) {
    if (!app) return;
    
    app->running = false;
    kprintf("JavierApp: Terminated '%s'\n", app->metadata.name);
}

/*
 * Unload application
 */
void javierapp_unload(javierapp_instance_t *app) {
    if (!app) return;
    
    app->running = false;
    
    if (app->main_window) {
        javier_window_destroy(app->main_window);
    }
    
    if (app->code_base) smart_free(app->code_base);
    if (app->data_base) smart_free(app->data_base);
    if (app->stack_base) smart_free(app->stack_base);
    
    smart_free(app);
}

/*
 * Get resource by type and ID
 */
const void *javierapp_get_resource(javierapp_instance_t *app, 
                                    resource_type_t type, uint32_t id) {
    if (!app) return NULL;
    
    /* Would search resource table */
    (void)type;
    (void)id;
    
    return NULL;
}

/*
 * Get resource by name
 */
const void *javierapp_get_resource_by_name(javierapp_instance_t *app,
                                            const char *name) {
    if (!app || !name) return NULL;
    
    /* Would search resource table by name */
    (void)name;
    
    return NULL;
}

/* ========== BUILDER FUNCTIONS ========== */

javierapp_builder_t *javierapp_builder_create(void) {
    javierapp_builder_t *builder = smart_alloc(sizeof(javierapp_builder_t), ALLOC_ZERO);
    if (!builder) return NULL;
    
    builder->header.magic = JAVIERAPP_MAGIC;
    builder->header.version = JAVIERAPP_VERSION;
    builder->header.header_size = sizeof(javierapp_header_t);
    
    return builder;
}

void javierapp_builder_set_metadata(javierapp_builder_t *builder,
                                     const char *name, const char *desc,
                                     const char *author, const char *version) {
    if (!builder) return;
    
    strncpy(builder->metadata.name, name, sizeof(builder->metadata.name) - 1);
    strncpy(builder->metadata.description, desc, sizeof(builder->metadata.description) - 1);
    strncpy(builder->metadata.author, author, sizeof(builder->metadata.author) - 1);
    strncpy(builder->metadata.version, version, sizeof(builder->metadata.version) - 1);
}

void javierapp_builder_add_code(javierapp_builder_t *builder,
                                 const void *code, size_t size) {
    if (!builder || !code || size == 0) return;
    
    builder->code = smart_alloc(size, ALLOC_NORMAL);
    if (builder->code) {
        memcpy(builder->code, code, size);
        builder->code_size = size;
    }
}

void javierapp_builder_add_resource(javierapp_builder_t *builder,
                                     resource_type_t type, uint32_t id,
                                     const char *name, const void *data, size_t size) {
    if (!builder || builder->resource_count >= 64) return;
    
    (void)data;  /* Resource data would be copied in full implementation */
    
    javierapp_resource_t *res = &builder->resources[builder->resource_count];
    res->type = type;
    res->id = id;
    res->size = size;
    if (name) strncpy(res->name, name, sizeof(res->name) - 1);
    
    builder->resource_count++;
}

int javierapp_builder_write(javierapp_builder_t *builder, const char *path) {
    if (!builder || !path) return -1;
    
    /* Calculate offsets */
    uint32_t offset = sizeof(javierapp_header_t) + sizeof(javierapp_metadata_t);
    builder->header.code_offset = offset;
    builder->header.code_size = builder->code_size;
    offset += builder->code_size;
    
    builder->header.data_offset = offset;
    builder->header.data_size = builder->data_size;
    offset += builder->data_size;
    
    builder->header.res_offset = offset;
    builder->header.res_count = builder->resource_count;
    builder->header.total_size = offset;  /* Plus resources */
    
    /* Create file */
    fd_t fd = vfs_open(path, O_CREAT | O_WRONLY, 0755);
    if (fd < 0) return -1;
    
    /* Write header */
    vfs_write(fd, &builder->header, sizeof(javierapp_header_t));
    vfs_write(fd, &builder->metadata, sizeof(javierapp_metadata_t));
    
    /* Write code */
    if (builder->code && builder->code_size > 0) {
        vfs_write(fd, builder->code, builder->code_size);
    }
    
    /* Write data */
    if (builder->data && builder->data_size > 0) {
        vfs_write(fd, builder->data, builder->data_size);
    }
    
    vfs_close(fd);
    return 0;
}

void javierapp_builder_destroy(javierapp_builder_t *builder) {
    if (!builder) return;
    
    if (builder->code) smart_free(builder->code);
    if (builder->data) smart_free(builder->data);
    smart_free(builder);
}
