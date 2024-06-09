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

    usleep((rand() % 15500));
    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    sleep(5);
    assert(sys_close(tid, fd) == 0);
}


void test_sys_open_1_fds_in_many_tasks() {
__TST_START__


    tsk_id_t _tid;
    int rt = -1;
    int i = 0;
    int count = 0;
    int inc = 0;
    int task_count = 912;

    for(; i < task_count; i ++) {
        rt = task_create(&_tid, task_execute_func);
        assert(rt == 0);
    }

    /// by default the first fd==1
    count = sys_vfs_ref_count(_tid, 1);
    while(count < task_count) {
        count = sys_vfs_ref_count(_tid, 1);
        printf("%d..", count);
        fflush(stdout);
        sleep(1);
        inc ++;
        assert(inc < 10);
    }
    assert(count == task_count);
    printf("\n");

    inc = 0;
    count = sys_vfs_ref_count(_tid, 1);
    while(count > 0) {
        count = sys_vfs_ref_count(_tid, 1);
        printf("%d..", count);
        fflush(stdout);
        sleep(1);
        inc ++;
        assert(inc < 10);
    }
    assert(count == 0);
    printf("\n");

__TST_PASSED__
}
