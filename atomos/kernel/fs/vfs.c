/*
 * AtomOS - Virtual File System Implementation
 */

#include "vfs.h"
#include "../include/kernel.h"
#include "../mm/heap.h"
#include "../process/process.h"

/* Root of the filesystem */
vfs_node_t *vfs_root = NULL;

/* Registered filesystems */
#define MAX_FILESYSTEMS 16
static filesystem_t *filesystems[MAX_FILESYSTEMS];
static int num_filesystems = 0;

/* Mount points */
#define MAX_MOUNTS 32
typedef struct {
    char path[256];
    vfs_node_t *node;
    filesystem_t *fs;
} mount_point_t;

static mount_point_t mount_points[MAX_MOUNTS];
static int num_mounts = 0;

/* Open file table entry */
typedef struct {
    vfs_node_t *node;
    off_t offset;
    int flags;
    int refcount;
} open_file_t;

#define MAX_OPEN_FILES 1024
static open_file_t open_files[MAX_OPEN_FILES];

/*
 * Allocate a file descriptor
 */
static fd_t alloc_fd(void) {
    for (int i = 3; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].node == NULL) {
            return i;
        }
    }
    return -1;
}

/*
 * Get open file entry
 */
static open_file_t *get_open_file(fd_t fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return NULL;
    if (open_files[fd].node == NULL) return NULL;
    return &open_files[fd];
}

/*
 * Initialize VFS
 */
void vfs_init(void) {
    memset(filesystems, 0, sizeof(filesystems));
    memset(mount_points, 0, sizeof(mount_points));
    memset(open_files, 0, sizeof(open_files));
    
    kprintf("VFS initialized\n");
}

/*
 * Register a filesystem
 */
void vfs_register_fs(filesystem_t *fs) {
    if (num_filesystems < MAX_FILESYSTEMS) {
        filesystems[num_filesystems++] = fs;
        kprintf("Registered filesystem: %s\n", fs->name);
    }
}

/*
 * Normalize a path (resolve . and ..)
 */
char *vfs_normalize_path(const char *path) {
    static char normalized[512];
    char *parts[64];
    int num_parts = 0;
    
    /* Copy path */
    char temp[512];
    strncpy(temp, path, sizeof(temp) - 1);
    
    /* Handle relative paths */
    if (temp[0] != '/') {
        process_t *proc = process_get_current();
        if (proc) {
            char full[512];
            strcpy(full, proc->cwd);
            strcat(full, "/");
            strcat(full, temp);
            strcpy(temp, full);
        }
    }
    
    /* Split into parts */
    char *token = temp;
    char *next;
    while (*token) {
        while (*token == '/') token++;
        if (*token == '\0') break;
        
        next = token;
        while (*next && *next != '/') next++;
        
        if (*next) {
            *next = '\0';
            next++;
        }
        
        if (strcmp(token, ".") == 0) {
            /* Skip */
        } else if (strcmp(token, "..") == 0) {
            if (num_parts > 0) num_parts--;
        } else {
            parts[num_parts++] = token;
        }
        
        token = next;
    }
    
    /* Rebuild path */
    normalized[0] = '/';
    normalized[1] = '\0';
    
    for (int i = 0; i < num_parts; i++) {
        if (i > 0) strcat(normalized, "/");
        else if (normalized[strlen(normalized)-1] != '/') strcat(normalized, "/");
        strcat(normalized, parts[i]);
    }
    
    return normalized;
}

/*
 * Lookup a path
 */
vfs_node_t *vfs_lookup(const char *path) {
    if (!path || !vfs_root) return NULL;
    
    char *normalized = vfs_normalize_path(path);
    
    if (strcmp(normalized, "/") == 0) {
        return vfs_root;
    }
    
    vfs_node_t *node = vfs_root;
    char *token = normalized + 1;  /* Skip leading / */
    char *next;
    
    while (*token && node) {
        next = token;
        while (*next && *next != '/') next++;
        
        char saved = *next;
        *next = '\0';
        
        if (node->finddir) {
            node = node->finddir(node, token);
        } else {
            node = NULL;
        }
        
        *next = saved;
        if (*next) next++;
        token = next;
    }
    
    return node;
}

/*
 * Open a file
 */
fd_t vfs_open(const char *path, int flags, mode_t mode) {
    UNUSED mode_t m = mode;
    
    vfs_node_t *node = vfs_lookup(path);
    
    /* Create if needed and doesn't exist */
    if (!node && (flags & O_CREAT)) {
        char *normalized = vfs_normalize_path(path);
        
        /* Find parent directory */
        char parent_path[256];
        char filename[256];
        
        char *last_slash = strrchr(normalized, '/');
        if (last_slash == normalized) {
            strcpy(parent_path, "/");
            strcpy(filename, normalized + 1);
        } else if (last_slash) {
            strncpy(parent_path, normalized, last_slash - normalized);
            parent_path[last_slash - normalized] = '\0';
            strcpy(filename, last_slash + 1);
        } else {
            strcpy(parent_path, "/");
            strcpy(filename, normalized);
        }
        
        vfs_node_t *parent = vfs_lookup(parent_path);
        if (parent && parent->create) {
            parent->create(parent, filename, mode);
            node = vfs_lookup(path);
        }
    }
    
    if (!node) {
        return -1;
    }
    
    /* Call open if available */
    if (node->open) {
        int ret = node->open(node, flags);
        if (ret < 0) return ret;
    }
    
    /* Allocate file descriptor */
    fd_t fd = alloc_fd();
    if (fd < 0) return -1;
    
    open_files[fd].node = node;
    open_files[fd].offset = 0;
    open_files[fd].flags = flags;
    open_files[fd].refcount = 1;
    node->refcount++;
    
    /* Truncate if requested */
    if (flags & O_TRUNC) {
        node->size = 0;
    }
    
    /* Seek to end if append mode */
    if (flags & O_APPEND) {
        open_files[fd].offset = node->size;
    }
    
    return fd;
}

/*
 * Close a file
 */
int vfs_close(fd_t fd) {
    open_file_t *file = get_open_file(fd);
    if (!file) return -1;
    
    file->refcount--;
    
    if (file->refcount == 0) {
        if (file->node->close) {
            file->node->close(file->node);
        }
        file->node->refcount--;
        file->node = NULL;
    }
    
    return 0;
}

/*
 * Read from file
 */
ssize_t vfs_read(fd_t fd, void *buf, size_t count) {
    open_file_t *file = get_open_file(fd);
    if (!file) return -1;
    
    if (!file->node->read) return -1;
    
    ssize_t bytes = file->node->read(file->node, file->offset, count, buf);
    if (bytes > 0) {
        file->offset += bytes;
    }
    
    return bytes;
}

/*
 * Write to file
 */
ssize_t vfs_write(fd_t fd, const void *buf, size_t count) {
    open_file_t *file = get_open_file(fd);
    if (!file) return -1;
    
    if (!file->node->write) return -1;
    
    ssize_t bytes = file->node->write(file->node, file->offset, count, buf);
    if (bytes > 0) {
        file->offset += bytes;
    }
    
    return bytes;
}

/*
 * Seek in file
 */
off_t vfs_seek(fd_t fd, off_t offset, int whence) {
    open_file_t *file = get_open_file(fd);
    if (!file) return -1;
    
    off_t new_offset;
    
    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = file->offset + offset;
            break;
        case SEEK_END:
            new_offset = file->node->size + offset;
            break;
        default:
            return -1;
    }
    
    if (new_offset < 0) return -1;
    
    file->offset = new_offset;
    return new_offset;
}

/*
 * Get file status
 */
int vfs_stat(const char *path, stat_t *buf) {
    vfs_node_t *node = vfs_lookup(path);
    if (!node) return -1;
    
    if (node->stat) {
        return node->stat(node, buf);
    }
    
    /* Fill in default values */
    memset(buf, 0, sizeof(stat_t));
    buf->st_ino = node->inode;
    buf->st_mode = node->mask;
    buf->st_uid = node->uid;
    buf->st_gid = node->gid;
    buf->st_size = node->size;
    
    if (node->flags & VFS_DIRECTORY) {
        buf->st_mode |= 0040000;
    } else if (node->flags & VFS_FILE) {
        buf->st_mode |= 0100000;
    }
    
    return 0;
}

/*
 * Get file status by fd
 */
int vfs_fstat(fd_t fd, stat_t *buf) {
    open_file_t *file = get_open_file(fd);
    if (!file) return -1;
    
    if (file->node->stat) {
        return file->node->stat(file->node, buf);
    }
    
    memset(buf, 0, sizeof(stat_t));
    buf->st_ino = file->node->inode;
    buf->st_mode = file->node->mask;
    buf->st_size = file->node->size;
    
    return 0;
}

/*
 * Create directory
 */
int vfs_mkdir(const char *path, mode_t mode) {
    char *normalized = vfs_normalize_path(path);
    
    /* Find parent */
    char parent_path[256];
    char dirname[256];
    
    char *last_slash = strrchr(normalized, '/');
    if (last_slash == normalized) {
        strcpy(parent_path, "/");
        strcpy(dirname, normalized + 1);
    } else if (last_slash) {
        strncpy(parent_path, normalized, last_slash - normalized);
        parent_path[last_slash - normalized] = '\0';
        strcpy(dirname, last_slash + 1);
    } else {
        return -1;
    }
    
    vfs_node_t *parent = vfs_lookup(parent_path);
    if (!parent || !parent->mkdir_op) return -1;
    
    return parent->mkdir_op(parent, dirname, mode);
}

/*
 * Read directory entry
 */
dirent_t *vfs_readdir(fd_t fd) {
    open_file_t *file = get_open_file(fd);
    if (!file) return NULL;
    
    if (!(file->node->flags & VFS_DIRECTORY)) return NULL;
    if (!file->node->readdir) return NULL;
    
    dirent_t *entry = file->node->readdir(file->node, file->offset);
    if (entry) {
        file->offset++;
    }
    
    return entry;
}

/*
 * Delete file
 */
int vfs_unlink(const char *path) {
    char *normalized = vfs_normalize_path(path);
    
    /* Find parent */
    char parent_path[256];
    char filename[256];
    
    char *last_slash = strrchr(normalized, '/');
    if (last_slash == normalized) {
        strcpy(parent_path, "/");
        strcpy(filename, normalized + 1);
    } else if (last_slash) {
        strncpy(parent_path, normalized, last_slash - normalized);
        parent_path[last_slash - normalized] = '\0';
        strcpy(filename, last_slash + 1);
    } else {
        return -1;
    }
    
    vfs_node_t *parent = vfs_lookup(parent_path);
    if (!parent || !parent->unlink) return -1;
    
    return parent->unlink(parent, filename);
}

/*
 * Mount filesystem
 */
int vfs_mount(const char *source, const char *target, 
              const char *fstype, uint32_t flags) {
    UNUSED const char *src = source;
    UNUSED uint32_t f = flags;
    
    /* Find filesystem */
    filesystem_t *fs = NULL;
    for (int i = 0; i < num_filesystems; i++) {
        if (strcmp(filesystems[i]->name, fstype) == 0) {
            fs = filesystems[i];
            break;
        }
    }
    
    if (!fs) {
        kerror("Unknown filesystem: %s\n", fstype);
        return -1;
    }
    
    /* Mount */
    vfs_node_t *mount_node = fs->mount(source, target);
    if (!mount_node) return -1;
    
    /* Record mount point */
    if (num_mounts < MAX_MOUNTS) {
        strncpy(mount_points[num_mounts].path, target, 255);
        mount_points[num_mounts].node = mount_node;
        mount_points[num_mounts].fs = fs;
        num_mounts++;
    }
    
    /* If mounting root, set vfs_root */
    if (strcmp(target, "/") == 0) {
        vfs_root = mount_node;
    }
    
    kprintf("Mounted %s on %s (%s)\n", source ? source : "none", target, fstype);
    return 0;
}

/*
 * Unmount filesystem
 */
int vfs_umount(const char *target) {
    for (int i = 0; i < num_mounts; i++) {
        if (strcmp(mount_points[i].path, target) == 0) {
            if (mount_points[i].fs->unmount) {
                mount_points[i].fs->unmount(mount_points[i].node);
            }
            
            /* Remove from list */
            for (int j = i; j < num_mounts - 1; j++) {
                mount_points[j] = mount_points[j + 1];
            }
            num_mounts--;
            
            return 0;
        }
    }
    
    return -1;
}
