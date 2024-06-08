#include <stdlib.h>
#include <string.h>
#include "sys.h"
#include "arch.h"
#include "queue.h"
#include "defs.h"
#include "task.h"


__S_PURE__ int __get_1d_index(int fd) {
    return fd / ARCH_VFS_FDS_PER_BUCKET;
}

__S_PURE__ int __get_2d_index(int fd) {
    return fd % ARCH_VFS_FDS_PER_BUCKET;
}

static int inc_get_fd(task_struct_t* tsk) {
    return atomic_inc(&(tsk->ts_lfd));
}

static vfs_handle_t* __search_handle(tsk_id_t tid, int fd) {
    vfs_handle_t* ret_handle;
    task_struct_t* tsk = task_manager_get_task(tid);
    if( !tsk || tsk->ts_priv_tid != arch_task_get_private_tid() ) {
        arch_signal_kill();
        return NULL;
    }
    /// No need atomic for compare.
    if( fd > tsk->ts_lfd.counter ) {
        return NULL;
    }

    int i1 = __get_1d_index(fd);
    int i2 = __get_2d_index(fd);

    ret_handle = tsk->ts_handle_buckets[i1].handles[i2];
    return ret_handle;
}

/**
 * TODO: Opimize GLOBAL_LOCK to HASH_INDEX_LOCK
 *
 * <TSK_ISSOLATION_BEGIN>
 * Get the task pointer
 * Check return if fds limitation
 * <TSK_ISSOLATION_END>
 *
 * <VFS_CONCURRENT_SAFE_BEGIN>
 * vfs_file_search search by path
 * vfs_file_ref_create if RW_CREATE and not exist
 * <VFS_CONCURRENT_SAFE_END>
 *
 * <TSK_ISSOLATION_BEGIN>
 * Alloc new vfs_handle*
 * Pop fd from Queue or generate new fd.
 * Calculate idx_bucket, idx_handle by nfd
 * Save vfs_handle* to vfs_handle_bucket_t by idx_bucket, idx_handle
 * <TSK_ISSOLATION_END>
 *
 * <FILE_W_LOCK>
 * file->f_ref_count ++
 * <FILE_W_UNLOCK>
 *
 * return fd
 */
int sys_open(tsk_id_t tid, const char* path, unsigned int mode) {
    int i1 = -1;
    int i2 = -1;
    int nfd;
    task_struct_t* tsk = task_manager_get_task(tid);
    vfs_handle_bucket_t* ptr_bucket;
    vfs_handle_t* ptr_handle;
    unsigned long file_ptr_addr;

    if( !tsk || tsk->ts_priv_tid != arch_task_get_private_tid() ) {
        return -1;
    }

    /// Get collected fd from queue
    arch_spin_lock(&(tsk->ts_lock));

    nfd = queue_dequeue(tsk->ts_recyc_fds);

    arch_spin_unlock(&(tsk->ts_lock));

    if(nfd < 0) {
        nfd = inc_get_fd(tsk);
    }

    /// calculate 1d and 2d indexes
    i1 = __get_1d_index(nfd);
    i2 = __get_2d_index(nfd);

    ptr_bucket = &(tsk->ts_handle_buckets[i1]);

    ptr_handle = &(ptr_bucket->handles[i2]);
    ptr_handle->fd = nfd;
    ptr_handle->mode = mode;
    atomic_set(&(ptr_handle->read_pos), 0);

    /// No free() needed. vfs_file_ref_create guarantee the mem collect.
    if(vfs_file_get_or_create(path, &file_ptr_addr, mode)) {
        atomic_set(&(ptr_handle->valid), 0);
        return -1;
    }
    ptr_handle->ptr_ptr_file_addr = file_ptr_addr;
    atomic_set(&(ptr_handle->valid), 1);

    return nfd;
}

int sys_close(tsk_id_t tid, int fd) {
    vfs_handle_t* handle = __search_handle(tid, fd);
    if(!handle || !atomic_read(&(handle->valid))) {
        return -1;
    }
    atomic_dec(&(VFS_PA2P(handle->ptr_ptr_file_addr)->f_ref_count));
    atomic_set(&(handle->valid), 0);
}

int sys_read(tsk_id_t tid, int fd, char *buf, size_t len, unsigned long* pos) {
    vfs_handle_t* handle = __search_handle(tid, fd);
    int rt = 0;
    if(!handle || !atomic_read(&(handle->valid))) {
        return -1;
    }

    rt = vfs_read(VFS_PA2P(handle->ptr_ptr_file_addr), buf, len, atomic_read(&(handle->read_pos)));

    if(rt > 0) {
        atomic_add(&(handle->read_pos), rt);
    }
    return rt;
}

int sys_write(tsk_id_t tid, int fd, const char *buf, size_t len, unsigned long pos) {
    vfs_handle_t* handle = __search_handle(tid, fd);
    int rt = 0;
    if(!handle || !atomic_read(&(handle->valid))) {
        return -1;
    }
    rt = vfs_write(VFS_PA2P(handle->ptr_ptr_file_addr), buf, len, pos);
    return rt;
}

void sys_init() {
    if(vfs_sys_init()) {
        panic();
    }
    if(task_manager_init()) {
        panic();
    }
}

void panic() {
    arch_panic();
}


