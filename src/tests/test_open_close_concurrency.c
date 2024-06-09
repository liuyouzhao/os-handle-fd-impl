#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"
#include <stdlib.h>

static void* task_execute_func(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    sleep(1);
    assert(sys_close(tid, fd) == 0);
}


void test_sys_open_1_fds_in_many_tasks() {
__TST_START__

    int rt = -1;
    int i = 0;
    const int task_count = 1024;
    tsk_id_t tids[task_count];

    for(; i < task_count; i ++) {
        rt = task_create(&tids[i], task_execute_func);
        assert(rt == 0);
    }

    i = 0;
    for(; i < task_count; i ++) {
        task_destroy(tids[i]);
    }

__TST_PASSED__
}
