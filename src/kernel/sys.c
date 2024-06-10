#include <stdlib.h>
#include <string.h>
#include "sys.h"
#include "arch.h"
#include "queue.h"
#include "defs.h"
#include "task.h"


__S_PURE__ int __get_bucket_index(int fd) {
    return fd / ARCH_VFS_FDS_PER_BUCKET;
}

__S_PURE__ int __get_handle_index(int fd) {
    return fd % ARCH_VFS_FDS_PER_BUCKET;
}

/**
 * Fetch from 2d array table
 * TC O(1)
 *
 * <TSK_ISSOLATION_BEGIN>
 * Get the task pointer
 * <TSK_ISSOLATION_END>
 *
 * Calculate i_bucket, i_handle
 * Get handle lock by i_bucket, i_handle
 *
 * Get bucket without lock and check NULL
 * Get handle
 * <HANDLE_R_LOCK>
 *
 * <HANDLE_R_UNLOCK>
 */
static vfs_handle_t** __search_handle(task_struct_t* task, int fd) {
    vfs_handle_bucket_t* bucket;

    /// No need atomic for compare.
    if( fd > task->ts_lfd.counter ) {
        return NULL;
    }

    int i_bucket = __get_bucket_index(fd);
    int i_handle = __get_handle_index(fd);

    /// bucket and bucket->handles are never free(), so no need lock here.
    bucket = &(task->ts_handle_buckets[i_bucket]);
    if(!bucket->handles) {
        return NULL;
    }


    /// sys_close can be called here to release the specific handle.
    /// any time from here, the *ret_handle can be NULL or dirty memory
    /// but it is totally ok because we will have lock during read/write
    /// See {Checkpoint1:Dirty handle pointer}
    return &(task->ts_handle_buckets[i_bucket].handles[i_handle]);
}

static int __detach_handle(task_struct_t* task, int fd) {
    int i_bucket = __get_bucket_index(fd);
    int i_handle = __get_handle_index(fd);
    unsigned long priv_ptr_address;

    vfs_handle_t** ptr_ptr_handle = &(task->ts_handle_buckets[i_bucket].handles[i_handle]);

    if(*ptr_ptr_handle) {

#if ARCH_CONF_SUB_TASK_ENABLE

        /// handle w lock
        arch_rw_lock_w(&(task->ts_handle_buckets->handle_rw_locks[fd]));

        /// store the private address
        priv_ptr_address = (*ptr_ptr_handle)->ptr_ptr_file_addr;

        /// double lookup after lock acquired.
        if(*ptr_ptr_handle) {
            /// detach handle
            free(*ptr_ptr_handle);
            /// {Checkpoint1:Dirty handle pointer}
            *ptr_ptr_handle = NULL;
        }

        /// handle w unlock
        arch_rw_unlock_w(&(task->ts_handle_buckets->handle_rw_locks[fd]));

        /// recycle this detached fd to queue
        queue_enqueue(task->ts_recyc_fds, fd);

        /// file refernce - 1 by private arress
        vfs_file_ref_dec(priv_ptr_address);
#else
        free(*ptr_ptr_handle);
        *ptr_ptr_handle = NULL;
        queue_enqueue(task->ts_recyc_fds, fd);
#endif
        return 0;
    }
    return -1;
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
 * vfs_file_get_or_create
 * <VFS_CONCURRENT_SAFE_END>
 *
 * <TSK_ISSOLATION_BEGIN>
 * Pop fd from Queue or generate new fd.
 * Calculate idx_bucket, idx_handle by nfd
 * Alloc new vfs_handle*
 * Save vfs_handle* to vfs_handle_bucket_t by idx_bucket, idx_handle
 * <TSK_ISSOLATION_END>
 *
 * Associate file attributes to handle struct
 * During the association, if the file is removed, the file->valid==0,
 * the later read/write will lock file_rw_lock and check file->valid.
 *
 * close method is only disassociation, no need to check file status.
 * return fd
 */
int sys_open(tsk_id_t tid, const char* path, unsigned int mode) {
    int i_bucket = -1;
    int i_handle = -1;
    int nfd;
    int ret;
    task_struct_t* tsk = task_manager_get_task(tid);
    vfs_handle_bucket_t* ptr_bucket;
    vfs_handle_t* ptr_handle;
    unsigned long file_ptr_addr;

    if( tsk->ts_lfd.counter >= (int)ARCH_VFS_FDS_MAX ) {
        fprintf(stderr, "File %s open failed, fds exceed limit\n", path);
        return -1;
    }

    /// Get collected fd from queue
    nfd = queue_dequeue(tsk->ts_recyc_fds);

    if(nfd < 0) {
        nfd = atomic_inc(&(tsk->ts_lfd));
    }

    /// calculate 1d and 2d indexes
    i_bucket = __get_bucket_index(nfd);
    i_handle = __get_handle_index(nfd);

    /// Check collision
    ptr_bucket = &(tsk->ts_handle_buckets[i_bucket]);
    if(ptr_bucket->handles &&ptr_bucket->handles[i_handle]) {
        fprintf(stderr, "sys_open fd allocation conflicts fd=%d\n", nfd);
        fflush(stderr);
        return -1;
    }

    /// vfs create or get file
    ret = vfs_file_get_or_create(path, &file_ptr_addr, mode);
    if(ret) {
        return -1;
    }


    /// Check handles and alloc
    if(!ptr_bucket->handles) {
#if ARCH_CONF_SUB_TASK_ENABLE
        /// bucket w lock, re-use the first handle of its bucket as bucket lock
        /// TODO: Optimize using independent bucket locks
        /// re-using handle lock lets the first fd reading/writing be blocked during fd association.
        arch_rw_lock_w(&(tsk->ts_handle_buckets->handle_rw_locks[i_bucket * ARCH_VFS_FDS_PER_BUCKET]));

        /// double lookup after acquired lock
        if(!ptr_bucket->handles) {
            ptr_bucket->handles = (vfs_handle_t**) calloc(ARCH_VFS_FDS_PER_BUCKET, sizeof(vfs_handle_t*));
        }

        /// bucket w unlock
        arch_rw_unlock_w(&(tsk->ts_handle_buckets->handle_rw_locks[i_bucket * ARCH_VFS_FDS_PER_BUCKET]));
#else
        ptr_bucket->handles = (vfs_handle_t**) calloc(ARCH_VFS_FDS_PER_BUCKET, sizeof(vfs_handle_t*));
#endif
    }


    /// Pure process start
    ptr_handle = (vfs_handle_t*) malloc(sizeof(vfs_handle_t));
    ptr_handle->fd = nfd;
    ptr_handle->mode = mode;
    ptr_handle->read_pos = 0;
    ptr_handle->ptr_ptr_file_addr = file_ptr_addr;
    arch_spin_lock_init(&(ptr_handle->read_pos_lock));
    /// Pure process end

#if ARCH_CONF_SUB_TASK_ENABLE

    /// handle w lock
    arch_rw_lock_w(&(tsk->ts_handle_buckets->handle_rw_locks[nfd]));

    ptr_bucket->handles[i_handle] = ptr_handle;

    /// handle w unlock
    arch_rw_unlock_w(&(tsk->ts_handle_buckets->handle_rw_locks[nfd]));

#else
    ptr_bucket->handles[i_handle] = ptr_handle;
#endif

    return nfd;
}

int sys_close(tsk_id_t tid, int fd) {
    task_struct_t* task = task_manager_get_task(tid);

    /// any time from here, the *handle can be released.
    /// So if considering sub tasks parallism, handle lock is needed.
    if(__detach_handle(task, fd)) {
        /// fd not found(not an open fd)
        perror("fd is not valid\n");
        return -1;
    }
    return 0;
}


int sys_read(tsk_id_t tid, int fd, char *buf, size_t len, unsigned long* pos) {
    task_struct_t* task;
    vfs_handle_t** ptr_ptr_handle;
    int i_bucket = __get_bucket_index(fd);
    if(!pos) {
        return -1;
    }
    task = task_manager_get_task(tid);
    ptr_ptr_handle = __search_handle(task, fd);
    int tmp_pos = 0;
    int rt = -1;
    if ( !(*ptr_ptr_handle) ) {
        perror("fd handle is null\n");
        return -1;
    }

#if ARCH_CONF_SUB_TASK_ENABLE

    arch_rw_lock_r(&(task->ts_handle_buckets->handle_rw_locks[fd]));

    /// double lookup after lock acquired
    /// Still not NULL, then read
    if( *(ptr_ptr_handle) ) {

        /// read_pos may have already been changed by other sub-tasks
        /// who also use the same fd handle to read
        /// but it is fine, save the latest pos.
        tmp_pos = (*ptr_ptr_handle)->read_pos;

        /// since here, read_pos can be changed any time by other sub-tasks.
        /// who also use the same fd handle to read
        /// but we still use the tmp_pos
        rt = vfs_read(VFS_PA2P((*ptr_ptr_handle)->ptr_ptr_file_addr), buf, len, tmp_pos);

        /// lock for pos update, Only sub-tasks concurrently read same fd needs this lock
        arch_spin_lock(&((*ptr_ptr_handle)->read_pos_lock));
        if(rt > 0) {
            (*ptr_ptr_handle)->read_pos = tmp_pos + rt;
        }
        arch_spin_unlock(&((*ptr_ptr_handle)->read_pos_lock));
        /// unlock pos update

        *pos = (*ptr_ptr_handle)->read_pos;
    }
    arch_rw_unlock_r(&(task->ts_handle_buckets->handle_rw_locks[fd]));

    return rt;
#else
    rt = vfs_read(VFS_PA2P((*ptr_ptr_handle)->ptr_ptr_file_addr), buf, len, (*ptr_ptr_handle)->read_pos);

    if(rt > 0) {
        (*ptr_ptr_handle)->read_pos += rt;
        *pos = (*ptr_ptr_handle)->read_pos;
    }
#endif
    return rt;
}

int sys_seek(tsk_id_t tid, int fd, unsigned long pos) {
    task_struct_t* task;
    vfs_handle_t** ptr_ptr_handle;
    if(pos >= ARCH_CONF_VFS_BLOCK_SIZ) {
        perror("sys_seek position out of boundary.");
        return -1;
    }
    task = task_manager_get_task(tid);
    ptr_ptr_handle = __search_handle(task, fd);

    if( !ptr_ptr_handle || !(*ptr_ptr_handle) ) {
        return -1;
    }
    arch_spin_lock(&((*ptr_ptr_handle)->read_pos_lock));
    (*ptr_ptr_handle)->read_pos = pos;
    arch_spin_unlock(&((*ptr_ptr_handle)->read_pos_lock));

    return 0;
}

int sys_write(tsk_id_t tid, int fd, const char *buf, size_t len, unsigned long pos) {
    int i_bucket = __get_bucket_index(fd);
    task_struct_t* task = task_manager_get_task(tid);
    vfs_handle_t** ptr_ptr_handle = __search_handle(task, fd);
    int rt = 0;
    if ( !(*ptr_ptr_handle) ) {
        return -1;
    }

#if ARCH_CONF_SUB_TASK_ENABLE
    arch_rw_lock_r(&(task->ts_handle_buckets->handle_rw_locks[fd]));

    /// double lookup after lock acquired
    /// Still not NULL, then read
    if( *(ptr_ptr_handle) ) {
        rt = vfs_write(VFS_PA2P((*ptr_ptr_handle)->ptr_ptr_file_addr), (char*)buf, len, pos);
    }

    arch_rw_unlock_r(&(task->ts_handle_buckets->handle_rw_locks[fd]));
#else
    rt = vfs_write(VFS_PA2P((*ptr_ptr_handle)->ptr_ptr_file_addr), buf, len, pos);
#endif
    return rt;
}

int sys_vfs_ref_count(tsk_id_t tid, int fd) {
    task_struct_t* task = task_manager_get_task(tid);
    vfs_handle_t** ptr_ptr_handle = __search_handle(task, fd);
    if(!ptr_ptr_handle || !(*ptr_ptr_handle)) {
        return 0;
    }
    vfs_file_t* file = VFS_PA2P((*ptr_ptr_handle)->ptr_ptr_file_addr);
    return file->f_ref_count.counter;
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


