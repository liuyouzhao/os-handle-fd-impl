#ifndef TEST_DEF_H
#define TEST_DEF_H

#include <stdio.h>
#include "atomic.h"

#define __TST_START__ printf("%s begins\n", __FUNCTION__); fflush(stdout);

#define __TST_PASSED__ printf("%s PASSED\n\n\n", __FUNCTION__); fflush(stdout);

#define __TST_START_DEP__(dep) printf("\t%s --dep-->[%s] \n", __FUNCTION__, dep);
#define __TST_PASSED_DEP__ printf("\t%s PASSED\n", __FUNCTION__);

#define __tst_define__ static atomic_t __ato_done__;
#define __tst_follower_init__ atomic_init(&__ato_done__);
#define __tst_follower_done__ atomic_inc(&__ato_done__);

#define __tst_follower_wait__(n) \
while(atomic_read(&__ato_done__) < n) {\
sleep(1); \
}

static __inline__ long __DL(long v) {
    printf("[%s] dump -- %lu\n", __FUNCTION__, v);
    fflush(stdout);
    return v;
}

#endif // TEST_DEF_H
