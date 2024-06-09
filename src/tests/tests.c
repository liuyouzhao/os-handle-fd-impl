#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "sys.h"
#include "task.h"

void test_hash_function();
void test_task_create_one();
void test_task_create_three();
void test_task_create_many();
void test_create_many_files();
void test_same_file_reference();
void test_file_search();
void test_fd_reuse();
void test_sys_open_1_fds_in_many_tasks();
void test_read_write_single_task_sanity();
void test_read_write_same_file_from_4_tasks_1();
void test_read_write_same_file_from_4_tasks_2();
void test_read_write_diff_fds_4_subtasks();
void test_read_write_same_fd_4_subtasks();
void test_rlock_same_file_from_many_tasks();

int main()
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

    test_fd_reuse();

    test_sys_open_1_fds_in_many_tasks();

    test_read_write_single_task_sanity();

    test_read_write_same_file_from_4_tasks_1();

    test_read_write_same_file_from_4_tasks_2();

    test_read_write_diff_fds_4_subtasks();

    test_read_write_same_fd_4_subtasks();

    test_rlock_same_file_from_many_tasks();

    fflush(stdout);

    return 0;
}
