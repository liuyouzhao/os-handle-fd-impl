#ifndef TEST_DEF_H
#define TEST_DEF_H

#include <stdio.h>

#define __TST_START__ printf("%s begins\n", __FUNCTION__);
#define __TST_PASSED__ printf("%s PASSED\n", __FUNCTION__);

#define __TST_START_DEP__(dep) printf("\t%s --dep-->[%s] \n", __FUNCTION__, dep);
#define __TST_PASSED_DEP__ printf("\t%s PASSED\n", __FUNCTION__);

static __inline__ long __DL(long v) {
    printf("[%s] dump -- %lu\n", __FUNCTION__, v);
    fflush(stdout);
    return v;
}

#endif // TEST_DEF_H
