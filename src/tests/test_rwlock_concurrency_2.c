#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"
#include <stdlib.h>

#define TEST_BLK_RW_SIZ  1024

__tst_define__

static void* task_execute_read(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    char buf_read[TEST_BLK_RW_SIZ + 1] = {0};
    char buf_read_expect_x[TEST_BLK_RW_SIZ] = {0};
    char buf_read_expect_y[TEST_BLK_RW_SIZ] = {0};
    char buf_read_expect_z[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;
    int rounds = 20;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    memset(buf_read_expect_x, 'X', sizeof(buf_read_expect_x));
    memset(buf_read_expect_y, 'Y', sizeof(buf_read_expect_y));
    memset(buf_read_expect_z, 'Z', sizeof(buf_read_expect_z));

    while(rounds--) {
        while((rt = sys_read(tid, fd, buf_read, TEST_BLK_RW_SIZ, &pos)) > 0) {
            assert(strncmp(buf_read, buf_read_expect_x, rt) == 0
                   ||
                   strncmp(buf_read, buf_read_expect_y, rt) == 0
                   ||
                   strncmp(buf_read, buf_read_expect_z, rt) == 0
                   ||
                   strlen(buf_read) == 0);
            memset(buf_read, 0, sizeof(buf_read));
            expect_pos += rt;
            assert(expect_pos == pos);
        }
        sys_seek(tid, fd, 0);
        expect_pos = 0;
    }

    __tst_follower_done__
}

static void* task_execute_func_X(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    char buf_write[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    int rt = 0;
    int rounds = 2000;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    memset(buf_write, 'X', sizeof(buf_write));

    while(rounds) {
        rt = sys_write(tid, fd, buf_write, TEST_BLK_RW_SIZ, (pos += rt));
        if(rt <= 0) {
            pos = 0;
            rounds --;
        }
    }

    __tst_follower_done__
}

static void* task_execute_func_Y(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    char buf_write[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    int rt = 0;
    int rounds = 2000;
    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    memset(buf_write, 'Y', sizeof(buf_write));

    while(rounds) {
        rt = sys_write(tid, fd, buf_write, TEST_BLK_RW_SIZ, (pos += rt));
        if(rt <= 0) {
            pos = 0;
            rounds --;
        }
    }

    __tst_follower_done__
}

static void* task_execute_func_Z(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    char buf_write[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    int rt = 0;
    int rounds = 2000;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    memset(buf_write, 'Z', sizeof(buf_write));

    while(rounds) {
        rt = sys_write(tid, fd, buf_write, TEST_BLK_RW_SIZ, (pos += rt));
        if(rt <= 0) {
            pos = 0;
            rounds --;
        }
    }

    __tst_follower_done__
}

/**
 * Test 3 tasks writing and 1 task reading.
 * Validate the write-lock and read-lock
 */
void test_read_write_same_file_from_4_tasks_2() {
__TST_START__
    __tst_follower_init__

    int rt = -1;
    tsk_id_t tid_x;
    tsk_id_t tid_y;
    tsk_id_t tid_z;
    tsk_id_t tid_r;

    rt = task_create(&tid_x, task_execute_func_X);
    assert(rt == 0);
    rt = task_create(&tid_y, task_execute_func_Y);
    assert(rt == 0);
    rt = task_create(&tid_z, task_execute_func_Z);
    assert(rt == 0);
    rt = task_create(&tid_r, task_execute_read);
    assert(rt == 0);

    __tst_follower_wait__(4)
__TST_PASSED__
}
