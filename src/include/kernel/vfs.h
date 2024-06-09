#ifndef VFS_H
#define VFS_H

#include "atomic.h"
#include "hash.h"
#include "list.h"

#define VFS_PA2P(lu) (*((vfs_file_t**)lu))

typedef struct vfs_handle_s {

    int fd;
    atomic_long_t read_pos;
    unsigned long mode;
    unsigned long ptr_ptr_file_addr;

} vfs_handle_t;

typedef struct vfs_handle_bucket_s {

    vfs_handle_t** handles;

} vfs_handle_bucket_t;

/**
 * global shared file structure
 */
typedef struct vfs_file_s {

    char* path;
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

#if CONF_VFS_IDX_LST_ENBL
    list_node_t* p_files_list;
#endif
    arch_lock_t sys_lock;

} vfs_sys_t;

int vfs_sys_init();

int vfs_file_get_or_create(const char* path, unsigned long* file_ptr_addr, int create);
int vfs_file_delete(const char* path);
unsigned long vfs_file_ptr_addr_search(const char* path);
int vfs_files_hash_dump();
int vfs_files_list_dump();

int vfs_read(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos);
int vfs_write(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos);
#endif
