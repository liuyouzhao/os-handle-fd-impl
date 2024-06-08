#include <assert.h>
#include <stdlib.h>
#include "hash.h"
#include "test_def.h"

typedef unsigned long __t_hv_t;


void test_hash_function_pass() {
__TST_START__
    hash_map_t* map = hash_map_create(16);
    __t_hv_t ptr0_addr;
    __t_hv_t ptr1_addr;
    __t_hv_t ptr2_addr;
    __t_hv_t ptr3_addr;
    __t_hv_t ptr0_addr_2;
    __t_hv_t ptr1_addr_2;
    __t_hv_t ptr2_addr_2;
    __t_hv_t ptr3_addr_2;
    __t_hv_t ptr0_addr_from_insert;
    __t_hv_t ptr1_addr_from_insert;
    __t_hv_t ptr2_addr_from_insert;
    __t_hv_t ptr3_addr_from_insert;
    __t_hv_t* ptr_val0 = (__t_hv_t*)malloc(sizeof(__t_hv_t));
    __t_hv_t* ptr_val1 = (__t_hv_t*)malloc(sizeof(__t_hv_t));
    __t_hv_t* ptr_val2 = (__t_hv_t*)malloc(sizeof(__t_hv_t));
    __t_hv_t* ptr_val3 = (__t_hv_t*)malloc(sizeof(__t_hv_t));
    *ptr_val0 = 0;
    *ptr_val1 = 1;
    *ptr_val2 = 2;
    *ptr_val3 = 3;
    ptr0_addr_from_insert = hash_map_insert(map, "cam0", (__t_hv_t)ptr_val0);
    ptr1_addr_from_insert = hash_map_insert(map, "cam1", (__t_hv_t)ptr_val1);
    ptr2_addr_from_insert = hash_map_insert(map, "cam2", (__t_hv_t)ptr_val2);
    ptr3_addr_from_insert = hash_map_insert(map, "cam3", (__t_hv_t)ptr_val3);

    assert(hash_map_get(map, "cam0", &ptr0_addr) == 0);
    assert(hash_map_get(map, "cam1", &ptr1_addr) == 0);
    assert(hash_map_get(map, "cam2", &ptr2_addr) == 0);
    assert(hash_map_get(map, "cam3", &ptr3_addr) == 0);

    assert(*ptr_val0 == **((__t_hv_t**)ptr0_addr));
    assert(*ptr_val1 == **((__t_hv_t**)ptr1_addr));
    assert(*ptr_val2 == **((__t_hv_t**)ptr2_addr));
    assert(*ptr_val3 == **((__t_hv_t**)ptr3_addr));

    hash_map_get(map, "cam0", &ptr0_addr_2);
    hash_map_get(map, "cam1", &ptr1_addr_2);
    hash_map_get(map, "cam2", &ptr2_addr_2);
    hash_map_get(map, "cam3", &ptr3_addr_2);

    free(*((__t_hv_t**)ptr0_addr_2));
    free(*((__t_hv_t**)ptr1_addr_2));
    free(*((__t_hv_t**)ptr2_addr_2));
    free(*((__t_hv_t**)ptr3_addr_2));
    *((__t_hv_t**)ptr0_addr_2) = NULL;
    *((__t_hv_t**)ptr1_addr_2) = NULL;
    *((__t_hv_t**)ptr2_addr_2) = NULL;
    *((__t_hv_t**)ptr3_addr_2) = NULL;

    hash_map_get(map, "cam0", &ptr0_addr);
    hash_map_get(map, "cam1", &ptr1_addr);
    hash_map_get(map, "cam2", &ptr2_addr);
    hash_map_get(map, "cam3", &ptr3_addr);

    assert(NULL == *((__t_hv_t**)ptr0_addr));
    assert(NULL == *((__t_hv_t**)ptr1_addr));
    assert(NULL == *((__t_hv_t**)ptr2_addr));
    assert(NULL == *((__t_hv_t**)ptr3_addr));

    assert(NULL == *((__t_hv_t**)ptr0_addr_from_insert));
    assert(NULL == *((__t_hv_t**)ptr1_addr_from_insert));
    assert(NULL == *((__t_hv_t**)ptr2_addr_from_insert));
    assert(NULL == *((__t_hv_t**)ptr3_addr_from_insert));
__TST_PASSED__
}
