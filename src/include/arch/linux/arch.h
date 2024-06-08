#ifndef ARCH_H
#define ARCH_H

#include <pthread.h>


#define ARCH_VFS_FDS_MAX 1024lu
#define ARCH_VFS_FDS_BUCKETS_MAX 32lu
#define ARCH_VFS_FILE_BUCKETS_MAX 1024lu
#define ARCH_TSK_MAX 2048lu

#define TIMEOUT_NS 100000000lu
#define SPIN_TIME_NS 10000000lu

typedef unsigned long tsk_id_t;

typedef struct {
    pthread_mutex_t _mutex;
} arch_lock_t;

typedef struct {
    pthread_rwlock_t _rw_mutex;
    pthread_mutex_t _mutex;
} arch_rw_lock_t;


int arch_spin_lock_init(arch_lock_t* lock);
int arch_spin_lock_destroy(arch_lock_t* lock);
int arch_spin_lock(arch_lock_t* lock);
int arch_spin_unlock(arch_lock_t* lock);

int arch_rw_lock_init(arch_rw_lock_t* lock);
int arch_rw_lock_destroy(arch_rw_lock_t* lock);
int arch_rw_lock_r(arch_rw_lock_t* lock);
int arch_rw_lock_w(arch_rw_lock_t* lock);
int arch_rw_unlock_r(arch_rw_lock_t* lock);
int arch_rw_unlock_w(arch_rw_lock_t* lock);

void arch_panic();

void arch_signal_kill();

int arch_task_create(void *(*func)(void*), unsigned long* private_tid, unsigned long tid);
unsigned long arch_task_get_private_tid();

#endif
