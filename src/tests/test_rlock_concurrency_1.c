#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"
#include <stdlib.h>

__tst_define__

#define TEST_BLK_RW_SIZ  1024


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
            // printf("%s|", buf_read);
            // fflush(stdout);
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
        //printf("..\n");
        //fflush(stdout);
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
void test_rlock_same_file_from_many_tasks() {
__TST_START__

    int rt = -1;
    const int r_siz = 1024;
    const int w_siz = 512;
    tsk_id_t tid_x[w_siz];
    tsk_id_t tid_y[w_siz];
    tsk_id_t tid_z[w_siz];
    tsk_id_t tid_r[r_siz];
    int i = 0;

    __tst_follower_init__

    for(i = 0; i < w_siz; i ++) {
        rt = task_create(&tid_x[i], task_execute_func_X);
        assert(rt == 0);
        rt = task_create(&tid_y[i], task_execute_func_Y);
        assert(rt == 0);
        rt = task_create(&tid_z[i], task_execute_func_Z);
        assert(rt == 0);
    }

    for(i = 0; i < r_siz; i ++) {
        rt = task_create(&tid_r[i], task_execute_read);
    }
    assert(rt == 0);

    __tst_follower_wait__(4)

__TST_PASSED__
}
