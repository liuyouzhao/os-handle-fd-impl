#include <assert.h>
#include <string.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"

void test_file_create_success() {
    vfs_file_t *file = NULL;
    int rt = vfs_file_ref_create("/dev/test/testdev", &file);

    assert(rt == 0);

    assert(1 == atomic_read(&(file->f_ref_count)));
    assert(strcmp("/dev/test/testdev", file->path) == 0);
    assert(file->private_data != NULL);
}
