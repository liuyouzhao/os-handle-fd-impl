#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"

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

    printf("%s\n", buf_read);
    fflush(stdout);

    assert(strcmp(buf_write, buf_read) == 0);
    assert(sys_close(tid, fd) == 0);

    printf("Done\n");
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
    printf("\n");
}

static void* exec_loop_write_then_read_with_offset(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "YYYYYYY";
    char buf_read[1000] = {0};
    char buf_read_expect[1000] = {0};
    unsigned long pos = 0;
    unsigned long expect_pos = 0;
    int rt = 0;

    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    memset(buf_read_expect, 'Y', sizeof(buf_read_expect));

    while((rt = sys_write(tid, fd, buf_write, strlen(buf_write), (pos += rt))) > 0);
    while((rt = sys_read(tid, fd, buf_read, 1000, &pos)) > 0) {
        printf("%s|", buf_read);
        fflush(stdout);

        assert(strncmp(buf_read, buf_read_expect, rt) == 0);

        memset(buf_read, 0, sizeof(buf_read));

        expect_pos += rt;
        assert(expect_pos == pos);
    }
    printf("\n");
}


void test_read_write_single_task_sanity() {
    __TST_START__

    tsk_id_t _tid;
    int rt = -1;
    int i = 0;

    rt = task_create(&_tid, exec_single_write_then_read);
    assert(rt == 0);
    task_destroy(_tid);

    rt = task_create(&_tid, exec_loop_write_then_read_no_offset);
    assert(rt == 0);
    task_destroy(_tid);

    rt = task_create(&_tid, exec_loop_write_then_read_with_offset);
    task_destroy(_tid);

    __TST_PASSED__
}
