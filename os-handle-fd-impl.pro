TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

PLATFORM = linux

HEADERS += \
        src/include/arch/$$PLATFORM/*.h \
        src/include/kernel/*.h \
        src/include/kernel/defs.h \
        src/include/kernel/sys.h \
        src/include/kernel/task.h \
        src/include/lib/*.h \
        src/include/lib/list.h \
        src/tests/test_def.h

SOURCES += \
        src/arch/$$PLATFORM/arch.c \
        src/kernel/sys.c \
        src/kernel/task.c \
        src/kernel/vfs.c \
        src/lib/hash.c \
        src/lib/list.c \
        src/lib/queue.c \
        src/init.c \
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

INCLUDEPATH += \
        src/include \
        src/include/arch/$$PLATFORM \
        src/include/kernel \
        src/include/lib \
        src/tests/

LIBS += -lpthread

DISTFILES += \
    src/tests/aa
