#include <stdlib.h>
#include "defs.h"
#include "task.h"

static task_manager_t s_sys_task_manager;

int task_manager_get_count() {
    return atomic_read(&(s_sys_task_manager.count));
}

task_struct_t* task_manager_get_task(tsk_id_t task_id) {
    return s_sys_task_manager.tasks[task_id];
}

int task_manager_init() {
    const unsigned int len = ARCH_TSK_MAX * sizeof(task_struct_t);
    s_sys_task_manager.tasks = (task_struct_t**)calloc(len, sizeof(task_struct_t*));
    if(!s_sys_task_manager.tasks) {
        return -1;
    }
    atomic_set(&(s_sys_task_manager.count), 0);
    arch_spin_lock_init(&(s_sys_task_manager.lock));
    return 0;
}


__S_PURE__ task_struct_t* __task_alloc() {
    task_struct_t* __tsk = (task_struct_t*) malloc(sizeof(task_struct_t));
    int ret;
    if(!__tsk) {
        fprintf(stderr, "task_alloc failed as memory exceeded.\n");
        fflush(stderr);
        return NULL;
    }

    /// Do not set tid here.
    __tsk->ts_handle_buckets = NULL;
    __tsk->ts_recyc_fds = queue_create_queue();
    __tsk->ts_lfd.counter = 0;
    __tsk->ts_handle_buckets = (vfs_handle_bucket_t*) calloc(ARCH_VFS_FDS_BUCKETS_MAX, sizeof(vfs_handle_bucket_t));
    return __tsk;
}

task_struct_t* __task_free(task_struct_t** task) {
    task_struct_t* ptr_task = *task;
    free(ptr_task->ts_handle_buckets);
    queue_clean_queue(&(ptr_task->ts_recyc_fds));
    free(ptr_task);
    *task = NULL;
}

__S_PURE__ tsk_id_t __start_task(tsk_id_t tid, tsk_id_t* priv_tid, void *(*func)(void*)) {
    return arch_task_create(func, priv_tid, (void*)tid);
}

int task_create(tsk_id_t* out_task_id, void *(*func)(void*)) {
    int id = -1;
    tsk_id_t task_id;
    unsigned long prv_tid;
    task_struct_t* task;

    id = atomic_inc_limit(&(s_sys_task_manager.count), ARCH_TSK_MAX);
    if(id < 0) {
        fprintf(stderr, "Tasks number exceeded %lu \n", ARCH_TSK_MAX);
        fflush(stderr);
        return -1;
    }

    /// alloc in advance to use pure parallelly.
    task = __task_alloc();

    /// Most of the time there won't be errors, so __task_free won't be called frequently.
    arch_spin_lock(&(s_sys_task_manager.lock));

    task_id = (tsk_id_t) id - 1;
    if(s_sys_task_manager.tasks[task_id]) {
        fprintf(stderr, "Task creation failed, collision of kernel error. tid=");

        arch_spin_unlock(&(s_sys_task_manager.lock));
        __task_free(&task);
        return -1;
    }
    task->ts_tid = task_id;
    *out_task_id = task_id;
    if(__start_task(task_id, &prv_tid, func)) {
        fprintf(stderr, "Task creation failed, collision of kernel error. tid=");

        arch_spin_unlock(&(s_sys_task_manager.lock));
        __task_free(&task);
        return -1;
    }
    task->ts_priv_tid = prv_tid;

    s_sys_task_manager.tasks[task_id] = task;
    arch_spin_unlock(&(s_sys_task_manager.lock));

    return 0;
}

int task_destroy(tsk_id_t task_id) {
    /// Not Implemented. Out of question scope.
    return 0;
}

/**
 * Slow function. Not being used during run time.
 */
void task_manager_dump_tasks() {
    int i = 0;
    arch_spin_lock(&(s_sys_task_manager.lock));
    printf("Task Manager: %d tasks \n", atomic_read(&(s_sys_task_manager.count)));
    for(; i < atomic_read(&(s_sys_task_manager.count)); i ++) {
        printf("task->%u|%lu\n",
               s_sys_task_manager.tasks[i]->ts_priv_tid,
               atomic_read(&(s_sys_task_manager.tasks[i]->ts_lfd)));
    }
    arch_spin_unlock(&(s_sys_task_manager.lock));
}
