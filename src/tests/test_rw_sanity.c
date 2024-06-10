#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"

__tst_define__

#define __TEST_BUF_SIZ 256

static void* exec_single_write_then_read(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "TLIMSILN_HMMLDIGP_HLMBQW_HRMS_HGMATRP_FHNS";
    char buf_read[__TEST_BUF_SIZ] = {0};
    unsigned long pos = 0;

    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    sys_write(tid, fd, buf_write, strlen(buf_write), 0);

    assert(sys_read(tid, fd, buf_read, __TEST_BUF_SIZ, &pos) > 0);
    assert(strcmp(buf_write, buf_read) == 0);
    assert(sys_close(tid, fd) == 0);

    __tst_follower_done__
    return NULL;
}

static void* exec_loop_write_then_read_no_offset(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "PR__OFIH_HBYN_YKC_YWBDOEAIIIH__E";
    char buf_read[32] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;

    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    while((rt = sys_write(tid, fd, buf_write, strlen(buf_write), (pos += rt))) > 0);
    while((rt = sys_read(tid, fd, buf_read, 32, &pos)) > 0) {
        assert(strcmp(buf_read, buf_write) == 0);
        expect_pos += rt;
        assert(expect_pos == pos);
    }

    __tst_follower_done__
    return NULL;
}

static void* exec_loop_write_then_read_with_offset(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "YYYYYYY";
    char buf_read[1001] = {0};
    char buf_read_expect[1000] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;

    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    memset(buf_read_expect, 'Y', sizeof(buf_read_expect));

    while((rt = sys_write(tid, fd, buf_write, strlen(buf_write), (pos += rt))) > 0);
    while((rt = sys_read(tid, fd, buf_read, 1000, &pos)) > 0) {

        assert(strncmp(buf_read, buf_read_expect, rt) == 0);

        memset(buf_read, 0, sizeof(buf_read));

        expect_pos += rt;
        assert(expect_pos == pos);
    }

    __tst_follower_done__
    return NULL;
}

static void* exec_loop_write_then_read_with_offset_2(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "XXXXXXX";
    char buf_read[11] = {0};
    char buf_read_expect[10] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;

    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    memset(buf_read_expect, 'X', sizeof(buf_read_expect));

    while((rt = sys_write(tid, fd, buf_write, strlen(buf_write), (pos += rt))) > 0);
    while((rt = sys_read(tid, fd, buf_read, 10, &pos)) > 0) {
        assert(strncmp(buf_read, buf_read_expect, rt) == 0);

        memset(buf_read, 0, sizeof(buf_read));

        expect_pos += rt;
        assert(expect_pos == pos);
    }

    __tst_follower_done__
    return NULL;
}

static void* exec_loop_write_close_then_read(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write1 = "YYYYYYY";
    const char* buf_write2 = "XXXXXXX";
    char buf_read[1001] = {0};
    char buf_read_expect[1000] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;
    const char* filename1 = "/dev/devtest/block1";
    const char* filename2 = "/dev/devtest/block2";

    int fd1 = sys_open(tid, filename1, 1);
    int fd2 = sys_open(tid, filename2, 1);
    assert(fd1 > 0);
    assert(fd2 > 0);


    while((rt = sys_write(tid, fd1, buf_write1, strlen(buf_write1), (pos += rt))) > 0);
    pos = 0;
    while((rt = sys_write(tid, fd2, buf_write2, strlen(buf_write2), (pos += rt))) > 0);

    sys_close(tid, fd1);
    sys_close(tid, fd2);

    int fd11 = sys_open(tid, filename1, 1);
    int fd22 = sys_open(tid, filename2, 1);

    /// Test the fd queue cache
    assert(fd11 == fd1);
    assert(fd22 == fd2);

    expect_pos = 0;
    memset(buf_read_expect, 'Y', sizeof(buf_read_expect));
    while((rt = sys_read(tid, fd11, buf_read, 1000, &pos)) > 0) {

        assert(strncmp(buf_read, buf_read_expect, rt) == 0);

        memset(buf_read, 0, sizeof(buf_read));

        expect_pos += rt;
        assert(expect_pos == pos);
    }

    expect_pos = 0;
    memset(buf_read, 0, sizeof(buf_read));
    memset(buf_read_expect, 'X', sizeof(buf_read_expect));
    while((rt = sys_read(tid, fd22, buf_read, 1000, &pos)) > 0) {

        assert(strncmp(buf_read, buf_read_expect, rt) == 0);

        memset(buf_read, 0, sizeof(buf_read));

        expect_pos += rt;
        assert(expect_pos == pos);
    }

    __tst_follower_done__
    return NULL;
}


void test_read_write_single_task_sanity() {
    __TST_START__

    tsk_id_t _tid;
    int rt = -1;

    __tst_follower_init__

    rt = task_create(&_tid, exec_single_write_then_read);
    assert(rt == 0);

    rt = task_create(&_tid, exec_loop_write_then_read_no_offset);
    assert(rt == 0);

    rt = task_create(&_tid, exec_loop_write_then_read_with_offset);
    assert(rt == 0);

    rt = task_create(&_tid, exec_loop_write_then_read_with_offset_2);
    assert(rt == 0);

    rt = task_create(&_tid, exec_loop_write_close_then_read);
    assert(rt == 0);

    __tst_follower_wait__(5)

    __TST_PASSED__
}
