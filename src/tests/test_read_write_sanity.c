#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "task.h"
#include "test_def.h"


static void* task_execute_func(void* param) {
    tsk_id_t tid = (tsk_id_t) param;
    const char* buf_write = "Steven test...";
    char buf_read[256] = {0};
    unsigned long pos = 0;

    int fd = sys_open(tid, "/dev/devtest/sys_open_test_dev1", 1);
    assert(fd > 0);

    sys_write(tid, fd, buf_write, strlen(buf_write), 0);

    assert(sys_read(tid, fd, buf_read, 255, &pos) > 0);

    printf("%s\n", buf_read);
    fflush(stdout);

    assert(strcmp(buf_write, buf_read) == 0);
    assert(sys_close(tid, fd) == 0);

    printf("Done\n");
}


void test_read_write_single_task_sanity() {
    __TST_START__

    tsk_id_t _tid;
    int rt = -1;
    int i = 0;

    rt = task_create(&_tid, task_execute_func);
    assert(rt == 0);

    sleep(2);

    __TST_PASSED__
}
