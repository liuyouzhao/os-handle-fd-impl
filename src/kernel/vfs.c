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

int vfs_file_ref_create(const char* path, vfs_file_t** output) {

    vfs_file_t* ptr_file = NULL;
    unsigned long private_addr = 0;
    int miss = 0;

    arch_spin_lock(&(__vfs_sys->sys_lock));

    miss = hash_map_get(__vfs_sys->p_files_map, path, &private_addr);

    arch_spin_unlock(&(__vfs_sys->sys_lock));

    /// search the file by path
    if(!miss) {
        /// found
        ptr_file = (vfs_file_t*) private_addr;
        atomic_inc(&(ptr_file->f_ref_count));
        *output = ptr_file;
        return 0;
    }

    /// create new file
    ptr_file = (vfs_file_t*) malloc(sizeof(vfs_file_t));
    ptr_file->path = path;
    atomic_inc(&(ptr_file->f_ref_count));
    arch_rw_lock_init(&(ptr_file->f_rw_lock));

    /// TODO: init the private_ptr by driver implementation
    ptr_file->private_data = (char*) malloc(4096);

    /// insert to hashmap and list
    arch_spin_lock(&(__vfs_sys->sys_lock));

    hash_map_insert(__vfs_sys->p_files_map, path, (unsigned long)(ptr_file));

#if CONF_VFS_IDX_LST_ENBL
    __vfs_sys->p_files_list = list_append_node(__vfs_sys->p_files_list, (unsigned long)(ptr_file));
#endif

    arch_spin_unlock(&(__vfs_sys->sys_lock));

    *output = ptr_file;

    return 0;
}

int vfs_file_delete(const char* path, vfs_file_t** output) {
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
