#include <assert.h>
#include <unistd.h>
#include "sys.h"
#include "task.h"
#include "test_def.h"

__tst_define__

static void* task_execute_func(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    sleep(1);
    assert(sys_close(tid, fd) == 0);

    __tst_follower_done__
}


void test_sys_open_1_fds_in_many_tasks() {
__TST_START__

    int rt = -1;
    int i = 0;
    const int task_count = 1024;
    tsk_id_t tids[task_count];

    __tst_follower_init__

    for(; i < task_count; i ++) {
        rt = task_create(&tids[i], task_execute_func);
        assert(rt == 0);
    }

    __tst_follower_wait__(task_count)

__TST_PASSED__
}
