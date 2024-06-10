#include "arch.h"
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

int arch_spin_lock_init(arch_lock_t* lock) {
    int ret = pthread_mutex_init(&(lock->_mutex), NULL);
    if(ret) {
        perror("lock init failed, kernel panic.");
        exit(-1);
    }
    return ret;
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
    int ret = pthread_mutex_unlock(&(lock->_mutex));
    if(ret) {
        perror("unlock spin_lock failed");
    }
    return ret;
}


int arch_rw_lock_init(arch_rw_lock_t* rw_lock) {
    int rt = pthread_rwlock_init(&(rw_lock->_rw_mutex), NULL);
    if(rt) {
        perror("rw_lock init failed, kernel panic.");
        exit(-1);
    }
    rt = pthread_mutex_init(&(rw_lock->_mutex), NULL);
    return rt;
}

int arch_rw_lock_destroy(arch_rw_lock_t* rw_lock) {
    if ( pthread_rwlock_destroy(&(rw_lock->_rw_mutex)) ) {
        return -1;
    }
    return pthread_mutex_destroy(&(rw_lock->_mutex));
}

int arch_rw_lock_r(arch_rw_lock_t* rw_lock) {
    int ret = pthread_rwlock_rdlock(&(rw_lock->_rw_mutex));
    if(ret) {
        perror("rw_lock_r failed");
    }
    return ret;
}

int arch_rw_lock_w(arch_rw_lock_t* rw_lock) {
    if ( pthread_mutex_lock(&(rw_lock->_mutex)) ) {
        perror("rw_lock_w failed(1)");
        return -1;
    }
    if( pthread_rwlock_wrlock(&(rw_lock->_rw_mutex)) ) {
        perror("rw_lock_w failed(2)");
        return -1;
    }
    return 0;
}

int arch_rw_unlock_r(arch_rw_lock_t* rw_lock) {
    int ret = pthread_rwlock_unlock(&(rw_lock->_rw_mutex));
    if(ret) {
        perror("rw_unlock_r failed");
    }
    return ret;
}

int arch_rw_unlock_w(arch_rw_lock_t* rw_lock) {
    if ( pthread_rwlock_unlock(&(rw_lock->_rw_mutex)) ) {
        perror("rw_unlock_w failed(1)");
        return -1;
    }
    if ( pthread_mutex_unlock(&(rw_lock->_mutex)) ) {
        perror("rw_unlock_w failed(2)");
        return -1;
    }
    return 0;
}

int arch_task_create(void *(*func)(void*), unsigned long* private_tid, void* args) {
#if ARCH_CONF_MOCK_TASK
    *private_tid = rand();
#else
    pthread_t pthread_id;
    int ret = pthread_create(&pthread_id, NULL, func, args);
    if(ret) {
        return ret;
    }
    *private_tid = (unsigned long)pthread_id;
    return ret;
#endif
}

unsigned long arch_task_get_private_tid() {
    return pthread_self();
}

int arch_join(unsigned long priv_tid) {
    pthread_t pthread_id = (pthread_t)priv_tid;
    return pthread_join(pthread_id, NULL);
}

void arch_signal_kill() {
    pthread_kill(pthread_self(), SIGUSR1);
}

void arch_panic() {
    exit(-1);
}
