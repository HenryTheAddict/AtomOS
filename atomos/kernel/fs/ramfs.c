/*
 * AtomOS - RAM Filesystem
 * Simple in-memory filesystem for initial root
 */

#include "vfs.h"
#include "../include/kernel.h"
#include "../mm/heap.h"

/* RAM file data */
typedef struct ramfs_file {
    char name[256];
    uint8_t *data;
    size_t size;
    size_t capacity;
    uint32_t flags;
    mode_t mode;
    struct ramfs_file *children;
    struct ramfs_file *next;
    struct ramfs_file *parent;
} ramfs_file_t;

/* Root of ramfs */
static ramfs_file_t *ramfs_root_data = NULL;

/*
 * Create a new ramfs file
 */
static ramfs_file_t *ramfs_create_file(const char *name, uint32_t flags) {
    ramfs_file_t *file = (ramfs_file_t *)kcalloc(1, sizeof(ramfs_file_t));
    if (!file) return NULL;
    
    strncpy(file->name, name, sizeof(file->name) - 1);
    file->flags = flags;
    file->mode = (flags & VFS_DIRECTORY) ? 0755 : 0644;
    
    if (!(flags & VFS_DIRECTORY)) {
        file->capacity = 4096;
        file->data = (uint8_t *)kmalloc(file->capacity);
        file->size = 0;
    }
    
    return file;
}

/*
 * Find child by name
 */
static ramfs_file_t *ramfs_find_child(ramfs_file_t *dir, const char *name) {
    ramfs_file_t *child = dir->children;
    while (child) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next;
    }
    return NULL;
}

/*
 * Add child to directory
 */
static void ramfs_add_child(ramfs_file_t *dir, ramfs_file_t *child) {
    child->parent = dir;
    child->next = dir->children;
    dir->children = child;
}

/*
 * VFS node from ramfs file
 */
static vfs_node_t *ramfs_get_node(ramfs_file_t *file);

/*
 * Read from file
 */
static ssize_t ramfs_read(vfs_node_t *node, off_t offset, size_t size, void *buf) {
    ramfs_file_t *file = (ramfs_file_t *)node->impl;
    if (!file || !file->data) return -1;
    
    if (offset >= (off_t)file->size) return 0;
    
    size_t to_read = size;
    if (offset + to_read > file->size) {
        to_read = file->size - offset;
    }
    
    memcpy(buf, file->data + offset, to_read);
    return to_read;
}

/*
 * Write to file
 */
static ssize_t ramfs_write(vfs_node_t *node, off_t offset, size_t size, const void *buf) {
    ramfs_file_t *file = (ramfs_file_t *)node->impl;
    if (!file) return -1;
    
    /* Expand if needed */
    if (offset + size > file->capacity) {
        size_t new_cap = file->capacity * 2;
        while (new_cap < offset + size) new_cap *= 2;
        
        uint8_t *new_data = (uint8_t *)krealloc(file->data, new_cap);
        if (!new_data) return -1;
        
        file->data = new_data;
        file->capacity = new_cap;
    }
    
    memcpy(file->data + offset, buf, size);
    
    if (offset + size > file->size) {
        file->size = offset + size;
        node->size = file->size;
    }
    
    return size;
}

/*
 * Read directory entry
 */
static dirent_t *ramfs_readdir(vfs_node_t *node, uint32_t index) {
    static dirent_t entry;
    ramfs_file_t *dir = (ramfs_file_t *)node->impl;
    if (!dir) return NULL;
    
    ramfs_file_t *child = dir->children;
    uint32_t i = 0;
    
    while (child && i < index) {
        child = child->next;
        i++;
    }
    
    if (!child) return NULL;
    
    entry.d_ino = (ino_t)child;
    strncpy(entry.d_name, child->name, sizeof(entry.d_name) - 1);
    
    return &entry;
}

/*
 * Find directory entry
 */
static vfs_node_t *ramfs_finddir(vfs_node_t *node, const char *name) {
    ramfs_file_t *dir = (ramfs_file_t *)node->impl;
    if (!dir) return NULL;
    
    ramfs_file_t *child = ramfs_find_child(dir, name);
    if (!child) return NULL;
    
    return ramfs_get_node(child);
}

/*
 * Create file
 */
static int ramfs_create_op(vfs_node_t *node, const char *name, mode_t mode) {
    UNUSED mode_t m = mode;
    ramfs_file_t *dir = (ramfs_file_t *)node->impl;
    if (!dir) return -1;
    
    if (ramfs_find_child(dir, name)) return -1;
    
    ramfs_file_t *file = ramfs_create_file(name, VFS_FILE);
    if (!file) return -1;
    
    ramfs_add_child(dir, file);
    return 0;
}

/*
 * Create directory
 */
static int ramfs_mkdir_op(vfs_node_t *node, const char *name, mode_t mode) {
    UNUSED mode_t m = mode;
    ramfs_file_t *dir = (ramfs_file_t *)node->impl;
    if (!dir) return -1;
    
    if (ramfs_find_child(dir, name)) return -1;
    
    ramfs_file_t *newdir = ramfs_create_file(name, VFS_DIRECTORY);
    if (!newdir) return -1;
    
    ramfs_add_child(dir, newdir);
    return 0;
}

/*
 * Delete file
 */
static int ramfs_unlink_op(vfs_node_t *node, const char *name) {
    ramfs_file_t *dir = (ramfs_file_t *)node->impl;
    if (!dir) return -1;
    
    ramfs_file_t *prev = NULL;
    ramfs_file_t *child = dir->children;
    
    while (child) {
        if (strcmp(child->name, name) == 0) {
            if (prev) {
                prev->next = child->next;
            } else {
                dir->children = child->next;
            }
            
            if (child->data) kfree(child->data);
            kfree(child);
            return 0;
        }
        prev = child;
        child = child->next;
    }
    
    return -1;
}

/*
 * Create VFS node from ramfs file
 */
static vfs_node_t *ramfs_get_node(ramfs_file_t *file) {
    vfs_node_t *node = (vfs_node_t *)kcalloc(1, sizeof(vfs_node_t));
    if (!node) return NULL;
    
    strncpy(node->name, file->name, sizeof(node->name) - 1);
    node->mask = file->mode;
    node->flags = file->flags;
    node->inode = (ino_t)file;
    node->size = file->size;
    node->impl = (uint32_t)file;
    
    node->read = ramfs_read;
    node->write = ramfs_write;
    node->readdir = ramfs_readdir;
    node->finddir = ramfs_finddir;
    node->create = ramfs_create_op;
    node->mkdir_op = ramfs_mkdir_op;
    node->unlink = ramfs_unlink_op;
    
    return node;
}

/*
 * Mount ramfs
 */
static vfs_node_t *ramfs_mount(const char *device, const char *mountpoint) {
    UNUSED const char *dev = device;
    UNUSED const char *mp = mountpoint;
    
    if (!ramfs_root_data) {
        ramfs_root_data = ramfs_create_file("/", VFS_DIRECTORY);
        
        /* Create basic directory structure */
        ramfs_file_t *bin = ramfs_create_file("bin", VFS_DIRECTORY);
        ramfs_file_t *dev_dir = ramfs_create_file("dev", VFS_DIRECTORY);
        ramfs_file_t *etc = ramfs_create_file("etc", VFS_DIRECTORY);
        ramfs_file_t *home = ramfs_create_file("home", VFS_DIRECTORY);
        ramfs_file_t *tmp = ramfs_create_file("tmp", VFS_DIRECTORY);
        ramfs_file_t *usr = ramfs_create_file("usr", VFS_DIRECTORY);
        ramfs_file_t *var = ramfs_create_file("var", VFS_DIRECTORY);
        
        ramfs_add_child(ramfs_root_data, bin);
        ramfs_add_child(ramfs_root_data, dev_dir);
        ramfs_add_child(ramfs_root_data, etc);
        ramfs_add_child(ramfs_root_data, home);
        ramfs_add_child(ramfs_root_data, tmp);
        ramfs_add_child(ramfs_root_data, usr);
        ramfs_add_child(ramfs_root_data, var);
    }
    
    return ramfs_get_node(ramfs_root_data);
}

/*
 * Unmount ramfs
 */
static int ramfs_unmount(vfs_node_t *node) {
    UNUSED vfs_node_t *n = node;
    /* Don't actually free - ramfs persists */
    return 0;
}

/* Ramfs filesystem type */
static filesystem_t ramfs_type = {
    .name = "ramfs",
    .mount = ramfs_mount,
    .unmount = ramfs_unmount
};

/*
 * Initialize ramfs
 */
void ramfs_init(void) {
    vfs_register_fs(&ramfs_type);
}
