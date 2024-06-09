#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "task.h"
#include "test_def.h"

void test_hash_function();
void test_task_create_one();
void test_task_create_three();
void test_task_create_many();
void test_create_many_files();
void test_same_file_reference();
void test_file_search();
void test_sys_open_1_fds_in_many_tasks();
void test_read_write_single_task_sanity();

int main(int argn, char** argc)
{

    printf("sys_init...\n");
    sys_init();
    printf("sys_init[OK]\n");

    task_manager_dump_tasks();

    test_hash_function();

    test_task_create_one();
    test_task_create_three();
    test_task_create_many();

    test_create_many_files();
    test_same_file_reference();
    test_file_search();

    test_sys_open_1_fds_in_many_tasks();

    test_read_write_single_task_sanity();

    fflush(stdout);

    return 0;
}
