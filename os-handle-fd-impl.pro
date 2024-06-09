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
        src/include/os/*.h \
        src/tests/test_def.h

SOURCES += \
        src/arch/$$PLATFORM/arch.c \
        src/kernel/sys.c \
        src/kernel/task.c \
        src/kernel/vfs.c \
        src/lib/hash.c \
        src/lib/list.c \
        src/lib/queue.c \
        src/os/dev.c \
        src/init.c \
        src/tests/lib/test_atomic.c \
        src/tests/lib/test_dentry.c \
        src/tests/lib/test_hash.c \
        src/tests/lib/test_queue.c \
        src/tests/test_file_open.c \
        src/tests/test_sys_open.c \
        src/tests/test_task_create.c \
        src/tests/test_vfs_file_create.c \
        src/tests/test_vfs_file_search.c \
        src/tests/tests.c

INCLUDEPATH += \
        src/include \
        src/include/arch/$$PLATFORM \
        src/include/kernel \
        src/include/lib \
        src/include/os \
        src/tests/

LIBS += -lpthread
