#include <stdlib.h>
#include <string.h>
#include "sys.h"
#include "arch.h"
#include "queue.h"

#define VFS_BUCKET_SIZE ARCH_MAX_FDS_BUCKETS
#define VFS_PER_BUCKET_FDS ARCH_MAX_FDS / ARCH_MAX_FDS_BUCKETS
#define MAX_TASK_NUM ARCH_MAX_PROC

static task_manager_t s_sys_task_manager;

/**
 * Slow function. Not being used during run time.
 */
void task_manager_dump_tasks() {
    int i = 0;
    arch_spin_lock(&(s_sys_task_manager.lock));
    printf("Task Manager: %d tasks \n", atomic_read(&(s_sys_task_manager.count)));
    for(; i < atomic_read(&(s_sys_task_manager.count)); i ++) {
        printf("task->%u|%lu\n",
               s_sys_task_manager.tasks[i].private_tid,
               atomic_read(&(s_sys_task_manager.tasks[i].t_lfd)));
    }
    arch_spin_unlock(&(s_sys_task_manager.lock));
}

int task_manager_get_count() {
    return atomic_read(&(s_sys_task_manager.count));
}

task_struct_t* task_manager_get_task(tsk_id_t task_id) {
    return &(s_sys_task_manager.tasks[task_id]);
}


static tsk_id_t next_tsk_id() {
    return (tsk_id_t) atomic_inc(&(s_sys_task_manager.count));
}

int task_manager_init() {
    const unsigned int len = MAX_TASK_NUM * sizeof(task_struct_t);
    s_sys_task_manager.tasks = (task_struct_t**)malloc(len);
    if(!s_sys_task_manager.tasks) {
        return -1;
    }
    else {
        memset(s_sys_task_manager.tasks, 0, len);
    }
    atomic_set(&(s_sys_task_manager.count), 0);
    arch_spin_lock_init(&(s_sys_task_manager.lock));
    return 0;
}

int task_create(tsk_id_t* task_id, void *(*func)(void*)) {
    unsigned long tid;
    int rt;

    /*
     * No need to add lock, concurrency supported by unit tsk_id.
    */
    *task_id = next_tsk_id() - 1;
    s_sys_task_manager.tasks[*task_id].handle_buckets = NULL;
    atomic_set(&(s_sys_task_manager.tasks[*task_id].t_lfd), 0);
    s_sys_task_manager.tasks[*task_id].t_recyc_fds = queue_create_queue();

    rt = arch_task_create(func, &tid, *task_id);
    if(rt) {
        task_destroy(*task_id);
        return -1;
    }
    s_sys_task_manager.tasks[*task_id].private_tid = tid;
    return 0;
}

int task_destroy(tsk_id_t task_id) {
    /// Not Implemented. Out of question scope.
    return 0;
}


static __inline__ int __get_1d_index(task_struct_t* tsk, int fd) {
    return fd / tsk->count_each_bucket;
}

static __inline__ int __get_2d_index(task_struct_t* tsk, int fd) {
    return fd % tsk->count_each_bucket;
}

static int inc_get_fd(task_struct_t* tsk) {
    return atomic_inc(&(tsk->t_lfd));
}

static vfs_handle_t* __search_handle(tsk_id_t tid, int fd) {
    task_struct_t* tsk = &(s_sys_task_manager.tasks[tid]);
    if( !tsk || tsk->private_tid != arch_task_get_private_tid() ) {
        arch_signal_kill();
    }
    int i1 = __get_1d_index(tsk, fd);
    int i2 = __get_2d_index(tsk, fd);
    return &(tsk->handle_buckets[i1].handles[i2]);
}

int sys_open(tsk_id_t tid, const char* path, unsigned int mode) {
    int i1 = -1;
    int i2 = -1;
    int nfd;
    task_struct_t* tsk = &(s_sys_task_manager.tasks[tid]);
    vfs_handle_bucket_t* ptr_bucket;
    vfs_handle_t* ptr_handle;
    vfs_file_t* file;

    if( !tsk || tsk->private_tid != arch_task_get_private_tid() ) {
        return -1;
    }

    /// init buckets if needed
    if(!tsk->handle_buckets) {

        arch_spin_lock(&(tsk->t_lock));

        /// double check regulation
        if(!tsk->handle_buckets) {
            tsk->handle_buckets = (vfs_handle_bucket_t*) malloc(sizeof(vfs_handle_bucket_t) * VFS_BUCKET_SIZE);
            tsk->count_each_bucket = VFS_PER_BUCKET_FDS;
            tsk->count_buckets = VFS_BUCKET_SIZE;
        }

        arch_spin_unlock(&(tsk->t_lock));
    }

    /// Get collected fd from queue
    arch_spin_lock(&(tsk->t_lock));

    nfd = queue_dequeue(tsk->t_recyc_fds);

    arch_spin_unlock(&(tsk->t_lock));

    if(nfd < 0) {
        nfd = inc_get_fd(tsk);
    }

    /// calculate 1d and 2d indexes
    i1 = __get_1d_index(tsk, nfd);
    i2 = __get_2d_index(tsk, nfd);

    /// init handles if needed
    ptr_bucket = &(tsk->handle_buckets[i1]);
    if(!ptr_bucket->handles) {

        arch_spin_lock(&(tsk->t_lock));

        /// double check regulation
        if(!ptr_bucket->handles) {
            ptr_bucket->handles = (vfs_handle_t*) malloc(sizeof(vfs_handle_t) * VFS_PER_BUCKET_FDS);
        }

        arch_spin_unlock(&(tsk->t_lock));
    }

    /// create fd reference
    ptr_handle = &(ptr_bucket->handles[i2]);
    ptr_handle->fd = nfd;
    ptr_handle->mode = mode;
    atomic_set(&(ptr_handle->read_pos), 0);

    /// No free() needed. vfs_file_ref_create guarantee the mem collect.
    if(vfs_file_ref_create(path, &file)) {
        atomic_set(&(ptr_handle->valid), 0);
        return -1;
    }
    ptr_handle->file = file;
    atomic_set(&(ptr_handle->valid), 1);

    return nfd;
}

int sys_close(tsk_id_t tid, int fd) {
    vfs_handle_t* handle = __search_handle(tid, fd);
    if(!handle || !atomic_read(&(handle->valid))) {
        return -1;
    }
    atomic_dec(&(handle->file->f_ref_count));
    atomic_set(&(handle->valid), 0);
}

int sys_read(tsk_id_t tid, int fd, char *buf, size_t len, unsigned long* pos) {
    vfs_handle_t* handle = __search_handle(tid, fd);
    int rt = 0;
    if(!handle || !atomic_read(&(handle->valid))) {
        return -1;
    }

    rt = vfs_read(handle->file, buf, len, atomic_read(&(handle->read_pos)));

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
    rt = vfs_write(handle->file, buf, len, pos);
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


