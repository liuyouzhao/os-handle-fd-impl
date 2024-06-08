#ifndef SYS_H
#define SYS_H

#include "vfs.h"
#include "queue.h"

typedef arch_lock_t task_spin_lock;
typedef arch_lock_t sys_lock;

typedef struct task_struct_s {

    tsk_id_t ts_tid;

    tsk_id_t ts_priv_tid;

    vfs_handle_bucket_t* ts_handle_buckets;

    atomic_long_t ts_lfd; /// Last fd value

    queue_t* ts_recyc_fds; /// recycled fds

    task_spin_lock ts_lock; /// task scope lock

    size_t ts_count_each_bucket;

    size_t ts_count_buckets;


} task_struct_t;

typedef struct task_manager_s {

    atomic_long_t count;
    task_struct_t** tasks;
    sys_lock lock;

} task_manager_t;

int task_manager_init();
void task_manager_dump_tasks();
int task_manager_get_count();
task_struct_t* task_manager_get_task_list();
task_struct_t* task_manager_get_task(tsk_id_t task_id);
int task_create(tsk_id_t* task_id, void *(*func)(void*));
int task_destroy(tsk_id_t task_id);

void sys_init();

int sys_open(tsk_id_t tid, const char* path, unsigned int mode);
int sys_close(tsk_id_t tid, int fd);
int sys_read(tsk_id_t tid, int fd, char *buf, size_t len, unsigned long* pos);
int sys_write(tsk_id_t tid, int fd, const char *buf, size_t len, unsigned long pos);

void panic();
#endif // SYS_H
