#include <assert.h>
#include <unistd.h>
#include "sys.h"
#include "test_def.h"

static tsk_id_t tmp_tid;

void* task_func_1(void *param) {
    tsk_id_t tid = (tsk_id_t) param;
    sleep(1);
    assert(tid == tmp_tid);

    while(1) {
        sleep(1);
    }
}

void* task_func_shared(void *param) {
    while(1) {
        sleep(1);
    }
}


void test_task_create_one_success() {
__TST_START__
    tsk_id_t _tid;
    int rt = task_create(&_tid, task_func_1);
    tmp_tid = _tid;

    assert(rt == 0);

    int count = task_manager_get_count();

    assert(count == 1);

    task_manager_dump_tasks();

__TST_PASSED__
}

void test_task_create_three_success() {
__TST_START__
    tsk_id_t _tid1;
    tsk_id_t _tid2;
    tsk_id_t _tid3;
    int rt = task_create(&_tid1, task_func_shared);
    assert(rt == 0);
    rt = task_create(&_tid2, task_func_shared);
    assert(rt == 0);
    rt = task_create(&_tid3, task_func_shared);

    int count = task_manager_get_count();

    assert(count == 4);

    task_manager_dump_tasks();

__TST_PASSED__
}