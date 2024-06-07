#include <assert.h>
#include "hash.h"


void test_hash_function_pass() {
    hash_map_t* map = hash_map_create(16);
    void *ptr0 = NULL;
    void *ptr1 = NULL;
    void *ptr2 = NULL;
    void *ptr3 = NULL;
    unsigned long val0 = 0;
    unsigned long val1 = 1;
    unsigned long val2 = 2;
    unsigned long val3 = 3;
    hash_map_insert(map, "cam0", (unsigned long)(&ptr0));
    hash_map_insert(map, "cam1", (unsigned long)(&ptr1));
    hash_map_insert(map, "cam2", (unsigned long)(&ptr2));
    hash_map_insert(map, "cam3", (unsigned long)(&ptr3));

    assert(hash_map_get(map, "cam0", &val0) == 1);
    assert(hash_map_get(map, "cam1", &val1) == 1);
    assert(hash_map_get(map, "cam2", &val2) == 1);
    assert(hash_map_get(map, "cam3", &val3) == 1);

    assert(val0 == (unsigned long)(&ptr0));
    assert(val1 == (unsigned long)(&ptr1));
    assert(val2 == (unsigned long)(&ptr2));
    assert(val3 == (unsigned long)(&ptr3));
}
