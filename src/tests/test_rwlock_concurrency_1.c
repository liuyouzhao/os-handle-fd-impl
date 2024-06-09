#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"
#include <stdlib.h>

static void* task_execute_read(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    char buf_read[8] = {0};
    char buf_read_expect_x[7] = {0};
    char buf_read_expect_y[7] = {0};
    char buf_read_expect_z[7] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;
    int run = 1;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    memset(buf_read_expect_x, 'X', sizeof(buf_read_expect_x));
    memset(buf_read_expect_y, 'Y', sizeof(buf_read_expect_y));
    memset(buf_read_expect_z, 'Z', sizeof(buf_read_expect_z));

    while(run) {
        run = 0;
        while((rt = sys_read(tid, fd, buf_read, 7, &pos)) > 0) {
            printf("%s|", buf_read);
            fflush(stdout);
            assert(strncmp(buf_read, buf_read_expect_x, rt) == 0
                   ||
                   strncmp(buf_read, buf_read_expect_y, rt) == 0
                   ||
                   strncmp(buf_read, buf_read_expect_z, rt) == 0
                   ||
                   strlen(buf_read) == 0);
            if(strlen(buf_read) == 0) {
                run = 1;
            }
            memset(buf_read, 0, sizeof(buf_read));
            expect_pos += rt;
            assert(expect_pos == pos);
        }
        printf("\n\n\n");
        fflush(stdout);
        sys_seek(tid, fd, 0);
        expect_pos = 0;
        usleep(1000 * 100);
    }
}

static void* task_execute_func_X(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "XXXXXXX";
    unsigned long pos = 0;
    int rt = 0;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    while((rt = sys_write(tid, fd, buf_write, strlen(buf_write), (pos += rt))) > 0) {
        /// wait 10ms
        usleep(1000 * 15);
    }
}

static void* task_execute_func_Y(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "YYYYYYY";
    unsigned long pos = 0;
    int rt = 0;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    while((rt = sys_write(tid, fd, buf_write, strlen(buf_write), (pos += rt))) > 0) {
        /// wait 10ms
        usleep(1000 * 12);
    }
}

static void* task_execute_func_Z(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "ZZZZZZZ";
    unsigned long pos = 0;
    int rt = 0;

    int fd = sys_open(tid, "/dev/devtest/same_block", 1);
    assert(fd > 0);

    while((rt = sys_write(tid, fd, buf_write, strlen(buf_write), (pos += rt))) > 0) {
        /// wait 10ms
        usleep(1000 * 10);
    }
}


void test_read_write_same_file_from_4_tasks_1() {
__TST_START__

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

    task_destroy(tid_x);
    task_destroy(tid_y);
    task_destroy(tid_z);
    task_destroy(tid_r);

__TST_PASSED__
}
