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

static tsk_id_t cross_tid = 9999;

static arch_lock_t lock;

static int cross_fd_check = -1;

static void* task_execute_read(void* param) {
    tsk_id_t tid = cross_tid;
    char buf_read[TEST_BLK_RW_SIZ + 1] = {0};
    char buf_read_expect_x[TEST_BLK_RW_SIZ] = {0};
    char buf_read_expect_y[TEST_BLK_RW_SIZ] = {0};
    char buf_read_expect_z[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;
    int rounds = 20;

    while(1) {
        arch_spin_lock(&lock);
        if(cross_fd_check >= 0) {
            arch_spin_unlock(&lock);
            break;
        }
        arch_spin_unlock(&lock);
    }

    memset(buf_read_expect_x, 'X', sizeof(buf_read_expect_x));
    memset(buf_read_expect_y, 'Y', sizeof(buf_read_expect_y));
    memset(buf_read_expect_z, 'Z', sizeof(buf_read_expect_z));

    while(rounds--) {
        while((rt = sys_read(tid, cross_fd_check, buf_read, TEST_BLK_RW_SIZ, &pos)) > 0) {
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
        sys_seek(tid, cross_fd_check, 0);
        expect_pos = 0;
        usleep(1000 * 200);
    }
}

static void* task_execute_func_X(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    char buf_write[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    int rt = 0;
    int rounds = 2000;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    arch_spin_lock(&lock);
    cross_fd_check = fd;
    arch_spin_unlock(&lock);

    memset(buf_write, 'X', sizeof(buf_write));

    while(rounds) {
        rt = sys_write(tid, fd, buf_write, TEST_BLK_RW_SIZ, (pos += rt));
        if(rt <= 0) {
            pos = 0;
            rounds --;
        }
        usleep(11);
    }
}

static void* task_execute_func_Y(void* param) {
    tsk_id_t tid = cross_tid;
    char buf_write[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    int rt = 0;
    int rounds = 2000;

    while(1) {
        arch_spin_lock(&lock);
        if(cross_fd_check >= 0) {
            arch_spin_unlock(&lock);
            break;
        }
        arch_spin_unlock(&lock);
    }

    memset(buf_write, 'Y', sizeof(buf_write));

    while(rounds) {
        rt = sys_write(tid, cross_fd_check, buf_write, TEST_BLK_RW_SIZ, (pos += rt));
        if(rt <= 0) {
            pos = 0;
            rounds --;
        }
        usleep(10);
    }
}

static void* task_execute_func_Z(void* param) {
    tsk_id_t tid = cross_tid;
    char buf_write[TEST_BLK_RW_SIZ] = {0};
    unsigned long pos = 0;
    int rt = 0;
    int rounds = 2000;

    while(1) {
        arch_spin_lock(&lock);
        if(cross_fd_check >= 0) {
            arch_spin_unlock(&lock);
            break;
        }
        arch_spin_unlock(&lock);
    }

    memset(buf_write, 'Z', sizeof(buf_write));

    while(rounds) {
        rt = sys_write(tid, cross_fd_check, buf_write, TEST_BLK_RW_SIZ, (pos += rt));
        if(rt <= 0) {
            pos = 0;
            rounds --;
        }
        usleep(10);
    }
}

/**
 * The scenario is for multiple sub-tasks access the same fd doing read/write.
 * As one level tasks-management, "task" is the smallest unit of concurrency.
 * This test is a simulation of sub-tasks cross-access the same fd.
 * Using different tasks access a same fd with shared-task-id can simulate this
 * scenario, even if access another task-id's resource is forbidden in this design.
 */
void test_read_write_same_fd_4_subtasks() {
__TST_START__

    int rt = -1;
    tsk_id_t tid_x;
    tsk_id_t tid_y;
    tsk_id_t tid_z;
    tsk_id_t tid_r;

    arch_spin_lock_init(&lock);

    rt = task_create(&tid_x, task_execute_func_X);
    assert(rt == 0);

    cross_tid = tid_x;

    rt = task_create(&tid_y, task_execute_func_Y);
    assert(rt == 0);
    rt = task_create(&tid_z, task_execute_func_Z);
    assert(rt == 0);
    rt = task_create(&tid_r, task_execute_read);
    assert(rt == 0);


    task_destroy(tid_y);
    task_destroy(tid_z);
    task_destroy(tid_r);
    /// destoy shared tasks at last
    task_destroy(tid_x);

    arch_spin_lock_destroy(&lock);

__TST_PASSED__
}
