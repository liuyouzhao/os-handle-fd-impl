#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"

#define __TEST_BUF_SIZ 256

__tst_define__

static void* single_task_open_close_fd_reuse(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const int test_fds = 512;
    int fds[test_fds];
    int i = 0;
    int tmp_fd;
    for(; i < test_fds; i ++) {
        fds[i] = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    }

    /// Close first half of test_fds
    for(i = 0; i < (test_fds >> 1); i ++) {
        sys_close(tid, fds[i]);
    }

    /// Open again
    for(i = 0; i < (test_fds >> 1); i ++) {
        tmp_fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
        assert(tmp_fd == fds[i]);
    }

    __tst_follower_done__

    return NULL;
}

void test_fd_reuse() {
    __TST_START__

    tsk_id_t _tid;
    int rt = -1;

    __tst_follower_init__

    rt = task_create(&_tid, single_task_open_close_fd_reuse);
    assert(rt == 0);

    __tst_follower_wait__(1)

    __TST_PASSED__
}
