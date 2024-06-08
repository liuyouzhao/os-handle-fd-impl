#include <assert.h>
#include <string.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "test_def.h"

void test_file_search_success() {
__TST_START_DEP__("test_create_many_files_success")
    int i = 0;
    vfs_file_t *file = NULL;
    char filename[256] = {0};

    for(; i < 1024; i ++) {
        memset(filename, 0, sizeof(filename));
        sprintf(filename, "/dev/testdev/testfd-%d", i);
        file = vfs_file_search(filename);
        assert(file != NULL);
        assert(1 == atomic_read(&(file->f_ref_count)));
        assert(strcmp(filename, file->path) == 0);
        assert(file->private_data != NULL);
    }

__TST_PASSED_DEP__
}
