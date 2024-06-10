#ifndef VFS_H
#define VFS_H

#include "atomic.h"
#include "hash.h"
#include "queue.h"

#define RW_ONLY 0
#define RW_CREATE 1

#define VFS_PA2P(lu) (*((vfs_file_t**)lu))

typedef struct vfs_handle_s {

    int fd;
    unsigned long read_pos;
    unsigned long mode;
    unsigned long ptr_ptr_file_addr;
    arch_lock_t read_pos_lock;

} vfs_handle_t;

typedef struct vfs_handle_bucket_s {

    vfs_handle_t** handles;

#if ARCH_CONF_SUB_TASK_ENABLE
    arch_rw_lock_t* handle_rw_locks;
#endif

} vfs_handle_bucket_t;

typedef struct vfs_file_s {

    char* path;
    atomic_t f_ref_count;
    arch_rw_lock_t f_rw_lock;
    unsigned long f_len;
    void* private_data;
    short valid;

} vfs_file_t;

typedef struct vfs_sys_s {

    hash_map_t* vfs_files_map;
    queue_t* vfs_files_recycle;
    arch_lock_t sys_lock;

} vfs_sys_t;

int vfs_sys_init();

int vfs_file_get_or_create(const char* path, unsigned long* file_ptr_addr, int create);
int vfs_file_delete(const char* path);
unsigned long vfs_file_ptr_addr_search(const char* path);
void vfs_file_ref_inc(unsigned long file_ptr_addr);
void vfs_file_ref_dec(unsigned long file_ptr_addr);
int vfs_files_hash_dump();
int vfs_files_list_dump();

int vfs_read(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos);
int vfs_write(vfs_file_t* file, const char* buf, unsigned long len, unsigned long pos);
#endif
