#include <assert.h>
#include <string.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "test_def.h"

void test_sys_open_many_fds_success() {
    int fd1 = sys_open(1, "/dev/devtest/sys_open_test_dev1", 1);
    int fd2 = sys_open(1, "/dev/devtest/sys_open_test_dev2", 1);
    int fd3 = sys_open(1, "/dev/devtest/sys_open_test_dev3", 1);
    int fd4 = sys_open(1, "/dev/devtest/sys_open_test_dev4", 1);
    int fd5 = sys_open(1, "/dev/devtest/sys_open_test_dev5", 1);
    int fd6 = sys_open(1, "/dev/devtest/sys_open_test_dev6", 1);
    int fd7 = sys_open(1, "/dev/devtest/sys_open_test_dev7", 1);

    assert(fd1 > 0);
    assert(fd2 > 0);
    assert(fd3 > 0);
    assert(fd4 > 0);
    assert(fd5 > 0);
    assert(fd6 > 0);
    assert(fd7 > 0);
}
