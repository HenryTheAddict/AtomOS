/*
 * AtomOS - Virtual File System
 * Unified interface for all filesystems
 */

#ifndef _ATOMOS_VFS_H
#define _ATOMOS_VFS_H

#include "../include/types.h"

/* File types */
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEVICE  0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE        0x05
#define VFS_SYMLINK     0x06
#define VFS_MOUNTPOINT  0x08

/* Open flags */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_NONBLOCK  0x0800
#define O_DIRECTORY 0x10000

/* Seek modes */
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

/* Forward declarations */
struct vfs_node;
struct dirent;

/* Directory entry */
typedef struct dirent {
    ino_t d_ino;
    char d_name[256];
} dirent_t;

/* File stat structure */
typedef struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    uint32_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
} stat_t;

/* Filesystem operations */
typedef ssize_t (*read_fn)(struct vfs_node *, off_t, size_t, void *);
typedef ssize_t (*write_fn)(struct vfs_node *, off_t, size_t, const void *);
typedef int (*open_fn)(struct vfs_node *, int flags);
typedef int (*close_fn)(struct vfs_node *);
typedef struct dirent *(*readdir_fn)(struct vfs_node *, uint32_t);
typedef struct vfs_node *(*finddir_fn)(struct vfs_node *, const char *);
typedef int (*create_fn)(struct vfs_node *, const char *, mode_t);
typedef int (*mkdir_fn)(struct vfs_node *, const char *, mode_t);
typedef int (*unlink_fn)(struct vfs_node *, const char *);
typedef int (*stat_fn)(struct vfs_node *, stat_t *);

/* VFS node structure */
typedef struct vfs_node {
    char name[256];
    uint32_t mask;          /* Permissions mask */
    uid_t uid;
    gid_t gid;
    uint32_t flags;         /* Node type flags */
    ino_t inode;
    off_t size;
    uint32_t impl;          /* Implementation-specific */
    
    /* Operations */
    read_fn read;
    write_fn write;
    open_fn open;
    close_fn close;
    readdir_fn readdir;
    finddir_fn finddir;
    create_fn create;
    mkdir_fn mkdir_op;
    unlink_fn unlink;
    stat_fn stat;
    
    /* For mountpoints */
    struct vfs_node *ptr;
    
    /* Reference count */
    uint32_t refcount;
} vfs_node_t;

/* Filesystem type */
typedef struct filesystem {
    const char *name;
    vfs_node_t *(*mount)(const char *device, const char *mountpoint);
    int (*unmount)(vfs_node_t *node);
} filesystem_t;

/* VFS functions */
void vfs_init(void);

/* File operations */
fd_t vfs_open(const char *path, int flags, mode_t mode);
int vfs_close(fd_t fd);
ssize_t vfs_read(fd_t fd, void *buf, size_t count);
ssize_t vfs_write(fd_t fd, const void *buf, size_t count);
off_t vfs_seek(fd_t fd, off_t offset, int whence);
int vfs_stat(const char *path, stat_t *buf);
int vfs_fstat(fd_t fd, stat_t *buf);

/* Directory operations */
int vfs_mkdir(const char *path, mode_t mode);
int vfs_rmdir(const char *path);
dirent_t *vfs_readdir(fd_t fd);

/* File management */
int vfs_unlink(const char *path);
int vfs_rename(const char *oldpath, const char *newpath);

/* Mount operations */
int vfs_mount(const char *source, const char *target, 
              const char *fstype, uint32_t flags);
int vfs_umount(const char *target);

/* Filesystem registration */
void vfs_register_fs(filesystem_t *fs);

/* Path operations */
vfs_node_t *vfs_lookup(const char *path);
char *vfs_normalize_path(const char *path);

/* Root filesystem */
extern vfs_node_t *vfs_root;

#endif /* _ATOMOS_VFS_H */
