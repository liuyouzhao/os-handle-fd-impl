#include <assert.h>
#include <string.h>
#include "hash.h"
#include "sys.h"
#include "vfs.h"
#include "test_def.h"

void test_create_many_files_success() {
    __TST_START__

    int i = 0;
    unsigned long file_ptr_address;
    char filename[256] = {0};
    int rt = 0;

    for(; i < 1024; i ++) {
        memset(filename, 0, sizeof(filename));
        sprintf(filename, "/dev/testdev/testfd-%d", i);
        rt = vfs_file_get_or_create(filename, &file_ptr_address, 1);
        assert(rt == 0);
        assert(1 == atomic_read(&(VFS_PA2P(file_ptr_address)->f_ref_count)));
        assert(strcmp(filename, VFS_PA2P(file_ptr_address)->path) == 0);
        assert(VFS_PA2P(file_ptr_address)->private_data != NULL);
    }

    vfs_files_hash_dump();
    vfs_files_list_dump();

    __TST_PASSED__
}

void test_same_file_reference_success() {
__TST_START__

    int i = 0;
    unsigned long file_ptr_address;
    unsigned long file_dup_ptr_address;
    int rt = vfs_file_get_or_create("/dev/test/testdev", &file_ptr_address, 1);

    assert(rt == 0);

    assert(1 == atomic_read(&(VFS_PA2P(file_ptr_address)->f_ref_count)));
    assert(strcmp("/dev/test/testdev", VFS_PA2P(file_ptr_address)->path) == 0);
    assert(VFS_PA2P(file_ptr_address)->private_data != NULL);

    /// reopen - 1
    rt = vfs_file_get_or_create("/dev/test/testdev", &file_dup_ptr_address, 1);

    assert(rt == 0);
    assert(VFS_PA2P(file_dup_ptr_address) == VFS_PA2P(file_ptr_address));
    assert(VFS_PA2P(file_dup_ptr_address)->private_data == VFS_PA2P(file_ptr_address)->private_data);
    assert(VFS_PA2P(file_dup_ptr_address)->f_len == VFS_PA2P(file_ptr_address)->f_len);
    assert(1 == atomic_read(&(VFS_PA2P(file_dup_ptr_address)->f_ref_count)));
    assert(1 == atomic_read(&(VFS_PA2P(file_ptr_address)->f_ref_count)));
    assert(&(VFS_PA2P(file_ptr_address)->f_rw_lock._mutex) == &(VFS_PA2P(file_dup_ptr_address)->f_rw_lock._mutex));
    assert(&(VFS_PA2P(file_ptr_address)->f_rw_lock._rw_mutex) == &(VFS_PA2P(file_dup_ptr_address)->f_rw_lock._rw_mutex));

    /// reopen - 2
    rt = vfs_file_get_or_create("/dev/test/testdev", &file_dup_ptr_address, 1);
    assert(rt == 0);
    assert(VFS_PA2P(file_dup_ptr_address) == VFS_PA2P(file_ptr_address));
    assert(VFS_PA2P(file_dup_ptr_address)->private_data == VFS_PA2P(file_ptr_address)->private_data);
    assert(VFS_PA2P(file_dup_ptr_address)->f_len == VFS_PA2P(file_ptr_address)->f_len);
    assert(1 == atomic_read(&(VFS_PA2P(file_dup_ptr_address)->f_ref_count)));
    assert(1 == atomic_read(&(VFS_PA2P(file_ptr_address)->f_ref_count)));
    assert(&(VFS_PA2P(file_ptr_address)->f_rw_lock._mutex) == &(VFS_PA2P(file_dup_ptr_address)->f_rw_lock._mutex));
    assert(&(VFS_PA2P(file_ptr_address)->f_rw_lock._rw_mutex) == &(VFS_PA2P(file_dup_ptr_address)->f_rw_lock._rw_mutex));

    /// reopen multiple times
    for(i = 0; i < 2048; i ++) {
        rt = vfs_file_get_or_create("/dev/test/testdev", &file_dup_ptr_address, 1);
        assert(rt == 0);
        assert(VFS_PA2P(file_dup_ptr_address) == VFS_PA2P(file_ptr_address));
        assert(VFS_PA2P(file_dup_ptr_address)->private_data == VFS_PA2P(file_ptr_address)->private_data);
        assert(VFS_PA2P(file_dup_ptr_address)->f_len == VFS_PA2P(file_ptr_address)->f_len);
        assert(1 == atomic_read(&(VFS_PA2P(file_dup_ptr_address)->f_ref_count)));
        assert(1 == atomic_read(&(VFS_PA2P(file_ptr_address)->f_ref_count)));
        assert(&(VFS_PA2P(file_ptr_address)->f_rw_lock._mutex) == &(VFS_PA2P(file_dup_ptr_address)->f_rw_lock._mutex));
        assert(&(VFS_PA2P(file_ptr_address)->f_rw_lock._rw_mutex) == &(VFS_PA2P(file_dup_ptr_address)->f_rw_lock._rw_mutex));

    }

    vfs_files_hash_dump();
    vfs_files_list_dump();

__TST_PASSED__
}
