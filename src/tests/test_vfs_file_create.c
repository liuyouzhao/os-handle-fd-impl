#include <assert.h>
#include <string.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "test_def.h"

void test_create_many_files_success() {
    __TST_START__

    int i = 0;
    vfs_file_t *file = NULL;
    char filename[256] = {0};
    int rt = 0;

    for(; i < 1024; i ++) {
        memset(filename, 0, sizeof(filename));
        sprintf(filename, "/dev/testdev/testfd-%d", i);
        rt = vfs_file_ref_create(filename, &file);
        assert(rt == 0);
        assert(1 == atomic_read(&(file->f_ref_count)));
        assert(strcmp(filename, file->path) == 0);
        assert(file->private_data != NULL);
    }

    vfs_files_hash_dump();
    vfs_files_list_dump();

    __TST_PASSED__
}

void test_same_file_reference_success() {
__TST_START__

    int i = 0;
    vfs_file_t *file = NULL;
    vfs_file_t *file_dup = NULL;
    int rt = vfs_file_ref_create("/dev/test/testdev", &file);

    assert(rt == 0);

    assert(1 == atomic_read(&(file->f_ref_count)));
    assert(strcmp("/dev/test/testdev", file->path) == 0);
    assert(file->private_data != NULL);

    /// reopen - 1
    rt = vfs_file_ref_create("/dev/test/testdev", &file_dup);

    assert(rt == 0);
    assert(file_dup == file);
    assert(file_dup->private_data == file->private_data);
    assert(file_dup->f_len == file->f_len);
    assert(2 == atomic_read(&(file_dup->f_ref_count)));
    assert(2 == atomic_read(&(file->f_ref_count)));
    assert(&(file->f_rw_lock._mutex) == &(file_dup->f_rw_lock._mutex));
    assert(&(file->f_rw_lock._rw_mutex) == &(file_dup->f_rw_lock._rw_mutex));

    /// reopen - 2
    rt = vfs_file_ref_create("/dev/test/testdev", &file_dup);
    assert(rt == 0);
    assert(file_dup == file);
    assert(file_dup->private_data == file->private_data);
    assert(file_dup->f_len == file->f_len);
    assert(3 == atomic_read(&(file_dup->f_ref_count)));
    assert(3 == atomic_read(&(file->f_ref_count)));
    assert(&(file->f_rw_lock._mutex) == &(file_dup->f_rw_lock._mutex));
    assert(&(file->f_rw_lock._rw_mutex) == &(file_dup->f_rw_lock._rw_mutex));

    /// reopen multiple times
    for(i = 0; i < 2048; i ++) {
        rt = vfs_file_ref_create("/dev/test/testdev", &file_dup);
        assert(rt == 0);
        assert(file_dup == file);
        assert(file_dup->private_data == file->private_data);
        assert(file_dup->f_len == file->f_len);
        assert(4 + i == atomic_read(&(file_dup->f_ref_count)));
        assert(4 + i == atomic_read(&(file->f_ref_count)));
        assert(&(file->f_rw_lock._mutex) == &(file_dup->f_rw_lock._mutex));
        assert(&(file->f_rw_lock._rw_mutex) == &(file_dup->f_rw_lock._rw_mutex));

    }

    vfs_files_hash_dump();
    vfs_files_list_dump();

__TST_PASSED__
}
