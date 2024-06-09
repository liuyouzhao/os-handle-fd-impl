##
 # Copyright (C) 2016 Rawcode Project. All rights reserved.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #   http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
##

C_FILES += \
	src/arch/$(PLATFORM)/arch.c \
        src/kernel/sys.c \
        src/kernel/task.c \
        src/kernel/vfs.c \
        src/lib/hash.c \
        src/lib/list.c \
        src/lib/queue.c \
        src/tests/lib/test_hash.c \
        src/tests/test_fd_reuse_sanity.c \
        src/tests/test_open_close_concurrency.c \
        src/tests/test_r1w3_subtasks_1.c \
        src/tests/test_r1w3_subtasks_2.c \
        src/tests/test_rlock_concurrency_1.c \
        src/tests/test_rw_sanity.c \
        src/tests/test_rwlock_concurrency_1.c \
        src/tests/test_rwlock_concurrency_2.c \
        src/tests/test_task_create.c \
        src/tests/test_vfs_file_create.c \
        src/tests/test_vfs_file_search.c \
        src/tests/tests.c
LD_LINER=

