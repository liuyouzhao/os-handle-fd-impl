#include <stdlib.h>
#include <string.h>
#include "vfs.h"
#include "hash.h"
#include "arch.h"
#include "defs.h"

static vfs_sys_t* __vfs_sys;

int vfs_sys_init() {
    __vfs_sys = (vfs_sys_t*) malloc(sizeof(vfs_sys_t));
    __vfs_sys->vfs_files_map = hash_map_create(ARCH_VFS_FILE_HASH_BUCKETS_SIZ);
    if(!__vfs_sys->vfs_files_map) {
        return -1;
    }

    __vfs_sys->vfs_files_recycle = queue_create_queue();
    if(!__vfs_sys->vfs_files_recycle) {
        return -1;
    }

    if(arch_spin_lock_init(&(__vfs_sys->sys_lock))) {
        return -1;
    }
    return 0;
}
__S_PURE__ vfs_file_t* __alloc_file(const char* path, int len) {
    vfs_file_t* ptr_file;

    ptr_file = (vfs_file_t*) malloc(sizeof(vfs_file_t));
    if(ptr_file == NULL) {
        fprintf(stderr, "%s failed. file malloc error. File %s\n", __FUNCTION__, path);
    }

    ptr_file->path = (char*) malloc(sizeof(char) * ARCH_VFS_FILENAME_MAX_LEN);
    if(ptr_file->path == NULL) {
        fprintf(stderr, "%s failed. path malloc error. File %s\n", __FUNCTION__, path);
    }

    memcpy(ptr_file->path, path, len * sizeof(char));
    if(arch_rw_lock_init(&(ptr_file->f_rw_lock))) {
        fprintf(stderr, "%s failed. lock init error. File %s\n", __FUNCTION__, path);
    }

    if(atomic_init(&(ptr_file->f_ref_count))) {
        fprintf(stderr, "%s failed. atomic init error. File %s\n", __FUNCTION__, path);
    }

    /// reference + 1, initially ref==1
    atomic_inc(&(ptr_file->f_ref_count));

    ptr_file->valid = 1;

    return ptr_file;
}

/**
 * [PURE] check path length limit
 *
 * vfs_file_ptr_addr_search
 *
 * <--- create/close concurrent intervenes
 *
 * Found:======================
 * return file;
 *
 * <--- create/close concurrent intervenes
 *
 * Not Found: =================
 *
 * [PURE] alloc new file
 *
 * <HASH_KEY_LOCK>
 * vfs_file_search (double lookup) NOT exist, or free and return
 * insert hashmap
 * <HASH_KEY_UNLOCK>
 * return file
 */
int vfs_file_get_or_create(const char* path, unsigned long* file_ptr_addr, int create) {
    vfs_file_t* ptr_file;
    size_t path_len = strlen(path);
    if(strlen(path) > ARCH_VFS_FILENAME_MAX_LEN) {
        fprintf(stderr, "vfs_file_ref_create path length exceeded max=%lu give=%lu\n", ARCH_VFS_FILENAME_MAX_LEN, path_len);
        return -1;
    }

    *file_ptr_addr = vfs_file_ptr_addr_search(path);

    /// found
    if(*file_ptr_addr) {
        ptr_file = VFS_PA2P(*file_ptr_addr);
        /// reference + 1, initially ref==1
        atomic_inc(&(ptr_file->f_ref_count));
        return 0;
    }
    else if(!create) {
        fprintf(stderr, "File not found. %s\n", path);
        return -1;
    }

    /// create new file
    ptr_file = __alloc_file(path, path_len);
    if(!ptr_file) {
        return -1;
    }

    /// TODO: init the private_ptr by driver ko implementation
    ptr_file->private_data = (char*) malloc(ARCH_CONF_VFS_BLOCK_SIZ);
    memset(ptr_file->private_data, 0, sizeof(char) * ARCH_CONF_VFS_BLOCK_SIZ);
    ptr_file->f_len = ARCH_CONF_VFS_BLOCK_SIZ;

    /// insert to hashmap
    /// Before insert, the file->ref > 0 already, the kernel task won't release it.
    *file_ptr_addr = hash_map_insert(__vfs_sys->vfs_files_map, ptr_file->path, (unsigned long)(ptr_file));
    return 0;
}

/// Normally O(1) operations. Hash collision can cause at worst O(n).
/// Using Optimized Tree-Redistribution Hash, the worst case O(logn)
/// <HASH_KEY_RW_LOCK>
/// hash_map_get
/// <HASH_KEY_RW_UNLOCK>
unsigned long vfs_file_ptr_addr_search(const char* path) {
    unsigned long file_pointer_addr = 0;
    if(hash_map_get(__vfs_sys->vfs_files_map, path, &file_pointer_addr)) {
        return (unsigned long)NULL;
    }
    return file_pointer_addr;
}

/**
 * vfs_file_search
 *
 * <--- create/close concurrent intervenes
 *
 * Not Found: =================
 * return -1
 *
 *
 * Found:======================
 * <FILE_W_LOCK>
 * check still valid==1 (double lookup) or return
 * Release all pointers except ref and lock
 * valid = 0
 * Save file pointer to QUEUE-{recycle}
 * <FILE_W_UNLOCK>
 *
 * <HASH_KEY_LOCK>
 * hash_map_remove (No need double check because remove itself will check)
 * <HASH_KEY_UNLOCK>
 */
int vfs_file_delete(const char* path) {
    unsigned long ptr_ptr_addr = vfs_file_ptr_addr_search(path);
    vfs_file_t* ptr_file = VFS_PA2P(ptr_ptr_addr);

    /// Not Found
    if(!ptr_file) {
        return -1;
    }

    arch_rw_lock_w(&(ptr_file->f_rw_lock));

    /// Double lookup after lock acquire
    if(!ptr_file->valid) {
        /// already deleted by another task
        return -1;
    }
    ptr_file->valid = 0;

    /// release internal buffer
    free(ptr_file->private_data);
    ptr_file->private_data = NULL;

    /// release filename(path) buffer
    free(ptr_file->path);
    ptr_file->path = NULL;

    /// Don't release f_ref_count and f_rw_lock
    /// When kernel task(thread) recycle the file object,
    /// it will free the f_rw_lock and vfs_file_t* when f_ref_count is 0.

    arch_rw_unlock_w(&(ptr_file->f_rw_lock));

    /// Remove from hashmap, key scope synchronized.
    /// Concurrenctly safe with
    /// vfs_file_ptr_addr_search and vfs_file_get_or_create
    hash_map_remove(__vfs_sys->vfs_files_map, path);

    /// Push the file address into global recyncle queue for kernel thread access
    queue_enqueue(__vfs_sys->vfs_files_recycle, ptr_ptr_addr);

    return 0;
}

void vfs_file_ref_inc(unsigned long file_ptr_addr) {
    vfs_file_t* file = VFS_PA2P(file_ptr_addr);
    atomic_inc(&(file->f_ref_count));
}

void vfs_file_ref_dec(unsigned long file_ptr_addr) {
    vfs_file_t* file = VFS_PA2P(file_ptr_addr);
    atomic_dec(&(file->f_ref_count));
}

static int __vfs_file_dump_key(unsigned long idx, const char* key, unsigned long data) {
    vfs_file_t* file = (vfs_file_t*) data;
    printf("__map(%lu)[%s]->%lu|%d|%p|%s\n",
           idx,
           key,
           file->f_len,
           file->f_ref_count.counter, file->private_data, file->path);
    return 0;
}

int vfs_files_hash_dump() {
    hash_map_dump(__vfs_sys->vfs_files_map, __vfs_file_dump_key);
    return 0;
}

int vfs_read(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos) {

    if(!file->valid) {
        return -1;
    }
    if(pos >= file->f_len) {
        return 0;
    }
    if(pos + len > file->f_len) {
        len = file->f_len - pos;
    }

    arch_rw_lock_r(&(file->f_rw_lock));

    /// TODO: hook user implemented driver ko ioctl(READ..) for scalability
    memcpy(buf, ((char*)file->private_data) + pos, len);

    arch_rw_unlock_r(&(file->f_rw_lock));

    return len;
}

int vfs_write(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos) {

    if(!file->valid) {
        return -1;
    }
    if(pos >= file->f_len) {
        return 0;
    }
    if(pos + len > file->f_len) {
        len = file->f_len - pos;
    }

    arch_rw_lock_w(&(file->f_rw_lock));

    /// TODO: hook user implemented driver ko ioctl(WRITE..) for scalability
    memcpy(((char*)file->private_data) + pos, buf, len);

    arch_rw_unlock_w(&(file->f_rw_lock));

    return len;
}
