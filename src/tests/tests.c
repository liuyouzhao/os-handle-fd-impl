#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "hash.h"
#include "sys.h"
#include "test_def.h"

void test_hash_function_pass();
void test_task_create_one_success();
void test_task_create_three_success();

int main(int argn, char** argc)
{

    printf("sys_init...\n");
    sys_init();
    printf("sys_init[OK]\n");

    task_manager_dump_tasks();

    test_hash_function_pass();

    test_task_create_one_success();
    test_task_create_three_success();

    fflush(stdout);
    sleep(3);
    return 0;
}
