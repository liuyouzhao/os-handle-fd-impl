#ifndef ATOMIC_H
#define ATOMIC_H

#include <asm/types.h>
#include "arch.h"

typedef struct {
    volatile int counter;
    arch_lock_t lock;
} atomic_t;

typedef atomic_t atomic_long_t;

#define atomic_inc_limit(v, l) (__atomic_add_return(   1, (v)  ))
#define atomic_inc(v) (__atomic_add_return(   1, (v)  ))
#define atomic_dec(v) (__atomic_add_return(  -1, (v)  ))
#define atomic_read(v) (__atomic_read(v))
#define atomic_set(v, i) ((void)(__atomic_set(   (v), i   )))
#define atomic_add(v, i) (__atomic_add_return(   i, (v)  ))

static __inline__ int __atomic_add_return_limit(int i, int l, atomic_t *v)
{
    int ret;
    arch_spin_lock(&(v->lock));

    if(v->counter + i <= l) {
        ret = (v->counter += i);
    }
    else {
        ret = -1;
    }

    arch_spin_unlock(&(v->lock));
    return ret;
}

static __inline__ int __atomic_add_return(int i, atomic_t *v)
{
    int ret;
    arch_spin_lock(&(v->lock));

    ret = (v->counter += i);

    arch_spin_unlock(&(v->lock));
    return ret;
}

static __inline__ int __atomic_read(const atomic_t *v)
{
    return v->counter;
}

static __inline__ void __atomic_set(atomic_t *v, int i)
{
    arch_spin_lock(&(v->lock));

    v->counter = i;

    arch_spin_unlock(&(v->lock));
}


#endif
