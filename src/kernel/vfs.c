#include <stdlib.h>
#include <string.h>
#include "vfs.h"
#include "hash.h"
#include "arch.h"

static vfs_sys_t* __vfs_sys;

int vfs_sys_init() {
    __vfs_sys = (vfs_sys_t*) malloc(sizeof(vfs_sys_t));
    __vfs_sys->p_files_map = hash_map_create(ARCH_VFS_FDS_BUCKETS_MAX);
    if(!__vfs_sys->p_files_map) {
        return -1;
    }
#if CONF_VFS_IDX_LST_ENBL
    __vfs_sys->p_files_list = NULL;
#endif
    if(arch_spin_lock_init(&(__vfs_sys->sys_lock))) {
        return -1;
    }
    return 0;
}

/**
 * [PURE] check path length limit
 *
 * <UNSAFE_BEGIN>
 * vfs_file_search
 * <UNSAFE_END>
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
        fprintf(stderr, "vfs_file_ref_create path length exceeded max=%lu give=%lu", ARCH_VFS_FILENAME_MAX_LEN, path_len);
        return -1;
    }

    *file_ptr_addr = vfs_file_ptr_addr_search(path);

    /// found
    if(*file_ptr_addr) {
        return 0;
    }
    else if(!create) {
        return -1;
    }

    /// create new file
    ptr_file = (vfs_file_t*) malloc(sizeof(vfs_file_t));
    ptr_file->path = (char*) malloc(sizeof(char) * ARCH_VFS_FILENAME_MAX_LEN);
    memcpy(ptr_file->path, path, path_len * sizeof(char));
    atomic_inc(&(ptr_file->f_ref_count));
    arch_rw_lock_init(&(ptr_file->f_rw_lock));

    /// TODO: init the private_ptr by driver implementation
    ptr_file->private_data = (char*) malloc(4096);

    /// insert to hashmap and list
    arch_spin_lock(&(__vfs_sys->sys_lock));

    *file_ptr_addr = hash_map_insert(__vfs_sys->p_files_map, ptr_file->path, (unsigned long)(ptr_file));

#if CONF_VFS_IDX_LST_ENBL
    __vfs_sys->p_files_list = list_append_node(__vfs_sys->p_files_list, (unsigned long)(ptr_file));
#endif
    arch_spin_unlock(&(__vfs_sys->sys_lock));

    return 0;
}

/// Normally O(1) operations. Hash collision can cause at worst O(n).
/// Using Optimized Tree-Redistribution Hash, the worst case O(logn)
unsigned long vfs_file_ptr_addr_search(const char* path) {
    unsigned long file_pointer_addr = 0;
    if(hash_map_get(__vfs_sys->p_files_map, path, &file_pointer_addr)) {
        return (unsigned long)NULL;
    }
    return file_pointer_addr;
}

/**
 * <UNSAFE_BEGIN>
 * vfs_file_search
 * <UNSAFE_END>
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
 * Release all pointers
 * free(file_address)
 * file_address = NULL
 * <FILE_W_UNLOCK>
 *
 * <HASH_KEY_LOCK>
 * hash_map_remove (No need double check because remove itself will check)
 * <HASH_KEY_UNLOCK>
 */
int vfs_file_delete(const char* path) {
    /// Not implemented, out of scope.
}

static int __vfs_file_dump_key(unsigned long idx, const char* key, unsigned long data) {
    vfs_file_t* file = (vfs_file_t*) data;
    printf("__map(%lu)[%s]->%lu|%d|%p|%s\n",
           idx,
           key,
           file->f_len,
           atomic_read(&(file->f_ref_count)), file->private_data, file->path);
    return 0;
}

static int __vfs_file_dump(unsigned long idx, unsigned long data) {
    vfs_file_t* file = (vfs_file_t*) data;
    printf("__lst(%lu)[%s]->%lu|%d|%p\n",
           idx,
           file->path,
           file->f_len,
           atomic_read(&(file->f_ref_count)), file->private_data);
    return 0;
}


int vfs_files_list_dump() {
#if CONF_VFS_IDX_LST_ENBL
    list_dump_list(__vfs_sys->p_files_list, __vfs_file_dump);
#endif
    return 0;
}

int vfs_files_hash_dump() {
    hash_map_dump(__vfs_sys->p_files_map, __vfs_file_dump_key);
    return 0;
}

int vfs_read(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos) {
    arch_rw_lock_r(&(file->f_rw_lock));

    if(pos >= file->f_len) {
        return 0;
    }
    if(pos + len > file->f_len) {
        len = file->f_len - pos;
    }

    memcpy(buf, ((char*)file->private_data) + pos, len);

    arch_rw_unlock_r(&(file->f_rw_lock));
    return len;
}

int vfs_write(vfs_file_t* file, char* buf, unsigned long len, unsigned long pos) {
    arch_rw_lock_w(&(file->f_rw_lock));

    if(pos >= file->f_len) {
        return 0;
    }
    if(pos + len > file->f_len) {
        len = file->f_len - pos;
    }

    memcpy(((char*)file->private_data) + pos, buf, len);

    arch_rw_unlock_w(&(file->f_rw_lock));
    return len;
}
