#ifndef ARCH_H
#define ARCH_H

#include <pthread.h>

#define TIMEOUT_NS 100000000
#define SPIN_TIME_NS 10000000

typedef struct {
    pthread_mutex_t _mutex;
} arch_lock_t;

int arch_spin_lock_init(arch_lock_t* lock);
int arch_spin_lock_destroy(arch_lock_t* lock);
int arch_spin_lock(arch_lock_t* lock);
int arch_spin_unlock(arch_lock_t* lock);

#endif
