#ifndef VFS_H
#define VFS_H

#include "atomic.h"
#include "hash.h"


typedef struct vfs_handle_s {
    int fd;
    atomic_long_t read_pos;
    unsigned long mode;
    atomic_long_t valid;

    struct vfs_file_s* file;
} vfs_handle_t;

typedef struct vfs_handle_bucket_s {

    vfs_handle_t* handles;

} vfs_handle_bucket_t;

/**
 * global shared file structure
 */
typedef struct vfs_file_s {

    const char* path;
    atomic_long_t f_ref_count;
    arch_rw_lock_t f_rw_lock;
    unsigned long f_len;
    void* private_data;
} vfs_file_t;

/**
 * global hashmap
 */
typedef struct vfs_sys_s {

    hash_map_t* p_files_map;
    arch_lock_t sys_lock;

} vfs_sys_t;

int vfs_sys_init();

int vfs_file_ref_create(const char* path, vfs_file_t** output);
int vfs_file_delete(const char* path, vfs_file_t** output);

int vfs_read(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos);
int vfs_write(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos);
#endif
