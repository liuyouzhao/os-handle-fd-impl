#ifndef TASK_H
#define TASK_H

#include "vfs.h"
#include "queue.h"

typedef struct task_struct_s {

    tsk_id_t ts_tid;

    tsk_id_t ts_priv_tid;

    vfs_handle_bucket_t* ts_handle_buckets;

    atomic_long_t ts_lfd; /// Last fd value

    queue_t* ts_recyc_fds; /// recycled fds

    arch_lock_t ts_lock; /// task scope lock

} task_struct_t;

typedef struct task_manager_s {

    atomic_long_t count;
    task_struct_t** tasks;
    arch_lock_t lock;

} task_manager_t;

int task_manager_init();
void task_manager_dump_tasks();
int task_manager_get_count();
task_struct_t* task_manager_get_task_list();
task_struct_t* task_manager_get_task(tsk_id_t task_id);
int task_create(tsk_id_t* task_id, void *(*func)(void*));
int task_destroy(tsk_id_t task_id);

#endif
