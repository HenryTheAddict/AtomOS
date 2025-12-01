/*
 * JavierApp Application Format
 * Native application format for AtomOS/Javier
 * 
 * File format (.javierapp):
 *   - Header (64 bytes)
 *   - Metadata section (variable)
 *   - Code section
 *   - Data section
 *   - Resources section (icons, images, etc.)
 */

#ifndef _JAVIERAPP_H
#define _JAVIERAPP_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"

/* Magic numbers */
#define JAVIERAPP_MAGIC     0x4A415650  /* "JAVP" */
#define JAVIERAPP_VERSION   1

/* Section types */
#define SECT_CODE       0x01
#define SECT_DATA       0x02
#define SECT_RODATA     0x03
#define SECT_RESOURCES  0x04
#define SECT_ICON       0x05
#define SECT_METADATA   0x06

/* App flags */
#define APP_FLAG_GUI        0x0001  /* Has GUI interface */
#define APP_FLAG_CLI        0x0002  /* Command line interface */
#define APP_FLAG_SERVICE    0x0004  /* Background service */
#define APP_FLAG_ADMIN      0x0008  /* Requires admin */
#define APP_FLAG_FULLSCREEN 0x0010  /* Fullscreen app */
#define APP_FLAG_LOWMEM     0x0020  /* Low memory mode */

/* Resource types */
typedef enum {
    RES_ICON_16,
    RES_ICON_32,
    RES_ICON_64,
    RES_IMAGE,
    RES_AUDIO,
    RES_STRING,
    RES_BINARY
} resource_type_t;

/* Application header */
typedef struct {
    uint32_t magic;             /* JAVIERAPP_MAGIC */
    uint32_t version;           /* Format version */
    uint32_t header_size;       /* Size of this header */
    uint32_t total_size;        /* Total file size */
    uint32_t flags;             /* App flags */
    uint32_t entry_point;       /* Entry point offset */
    uint32_t code_offset;       /* Code section offset */
    uint32_t code_size;         /* Code section size */
    uint32_t data_offset;       /* Data section offset */
    uint32_t data_size;         /* Data section size */
    uint32_t res_offset;        /* Resources offset */
    uint32_t res_count;         /* Number of resources */
    uint32_t min_memory;        /* Minimum memory required */
    uint32_t checksum;          /* CRC32 checksum */
    uint8_t reserved[8];        /* Reserved for future */
} __attribute__((packed)) javierapp_header_t;

/* Section header */
typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t size;
    uint32_t flags;
} __attribute__((packed)) javierapp_section_t;

/* Resource entry */
typedef struct {
    uint32_t type;
    uint32_t id;
    uint32_t offset;
    uint32_t size;
    char name[32];
} __attribute__((packed)) javierapp_resource_t;

/* Metadata */
typedef struct {
    char name[64];
    char description[256];
    char author[64];
    char version[32];
    uint32_t build_date;
    uint32_t min_os_version;
} javierapp_metadata_t;

/* Running application instance */
typedef struct {
    javierapp_header_t header;
    javierapp_metadata_t metadata;
    void *code_base;
    void *data_base;
    void *stack_base;
    uint32_t stack_size;
    javier_window_t *main_window;
    bool running;
    int exit_code;
} javierapp_instance_t;

/* Load and execute */
javierapp_instance_t *javierapp_load(const char *path);
int javierapp_run(javierapp_instance_t *app);
void javierapp_terminate(javierapp_instance_t *app);
void javierapp_unload(javierapp_instance_t *app);

/* Validation */
bool javierapp_validate(const void *data, size_t size);
uint32_t javierapp_checksum(const void *data, size_t size);

/* Resource access */
const void *javierapp_get_resource(javierapp_instance_t *app, 
                                    resource_type_t type, uint32_t id);
const void *javierapp_get_resource_by_name(javierapp_instance_t *app,
                                            const char *name);

/* App creation tools */
typedef struct {
    javierapp_header_t header;
    javierapp_metadata_t metadata;
    uint8_t *code;
    uint32_t code_size;
    uint8_t *data;
    uint32_t data_size;
    javierapp_resource_t resources[64];
    uint32_t resource_count;
} javierapp_builder_t;

javierapp_builder_t *javierapp_builder_create(void);
void javierapp_builder_set_metadata(javierapp_builder_t *builder,
                                     const char *name, const char *desc,
                                     const char *author, const char *version);
void javierapp_builder_add_code(javierapp_builder_t *builder,
                                 const void *code, size_t size);
void javierapp_builder_add_resource(javierapp_builder_t *builder,
                                     resource_type_t type, uint32_t id,
                                     const char *name, const void *data, size_t size);
int javierapp_builder_write(javierapp_builder_t *builder, const char *path);
void javierapp_builder_destroy(javierapp_builder_t *builder);

/* System call interface for apps */
typedef struct {
    void (*draw_pixel)(int x, int y, uint32_t color);
    void (*draw_line)(int x1, int y1, int x2, int y2, uint32_t color);
    void (*draw_rect)(int x, int y, int w, int h, uint32_t color);
    void (*draw_text)(int x, int y, const char *text, uint32_t color);
    void (*present)(void);
    int (*get_key)(void);
    int (*get_mouse)(int *x, int *y);
    void *(*alloc)(size_t size);
    void (*free)(void *ptr);
    int (*open_file)(const char *path, int mode);
    int (*read_file)(int fd, void *buf, size_t size);
    int (*write_file)(int fd, const void *buf, size_t size);
    void (*close_file)(int fd);
    void (*print)(const char *str);
    uint64_t (*get_time)(void);
    void (*sleep)(uint32_t ms);
    void (*exit)(int code);
} javierapp_api_t;

/* Get API table */
javierapp_api_t *javierapp_get_api(void);

#endif /* _JAVIERAPP_H */
