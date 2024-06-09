#ifndef ARCH_H
#define ARCH_H

#include <pthread.h>

/// fds
#define ARCH_VFS_FDS_MAX 1024lu
#define ARCH_VFS_FDS_BUCKETS_MAX 32lu
#define ARCH_VFS_FDS_PER_BUCKET (ARCH_VFS_FDS_MAX / ARCH_VFS_FDS_BUCKETS_MAX)

/// file
#define ARCH_VFS_FILE_HASH_BUCKETS_SIZ 1024lu
#define ARCH_VFS_FILENAME_MAX_LEN 512lu

#define ARCH_TSK_MAX 4096lu

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

int arch_task_create(void *(*func)(void*), unsigned long* private_tid, void* args);
int arch_join(unsigned long priv_tid);
unsigned long arch_task_get_private_tid();

//// Arch configs
#define ARCH_CONF_SUB_TASK_ENABLE 1
#define ARCH_CONF_MOCK_TASK 0
#define ARCH_CONF_VFS_BLOCK_SIZ 4096

#endif
