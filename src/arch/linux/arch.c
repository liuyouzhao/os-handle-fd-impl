#include "arch.h"

int arch_spin_lock_init(arch_lock_t* lock) {
    return pthread_mutex_init(&(lock->_mutex), NULL);
}

int arch_spin_lock_destroy(arch_lock_t* lock) {
    return pthread_mutex_destroy(&(lock->_mutex));
}

int arch_spin_lock(arch_lock_t* lock) {
    int rounds = TIMEOUT_NS / SPIN_TIME_NS;
    struct timespec ts;
    ts.tv_nsec = SPIN_TIME_NS;

    do {
        if(!pthread_mutex_timedlock(&(lock->_mutex), &ts)) {
            return 0;
        }
    } while(--rounds);
    return -1;
}

int arch_spin_unlock(arch_lock_t* lock) {
    return pthread_mutex_unlock(&(lock->_mutex));
}
