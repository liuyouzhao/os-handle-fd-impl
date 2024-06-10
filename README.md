# Virtual File System (VFS) Design and Implementation

# 1. Major Modules

## 1.1 VFS
The Virtual File System (VFS) is designed for efficient and concurrently safe path file search.

#### VFS File Structure
- In the code, a file is represented by the `vfs_file_t` struct.
- It is globally scoped, making it accessible across tasks.
- The primary goal of `vfs_file_t` is to balance efficiency and concurrency safety.

```c
typedef struct vfs_file_s {

    char* path;                /// file path
    atomic_t f_ref_count;      /// file ref
    arch_rw_lock_t f_rw_lock;  /// file level lock
    unsigned long f_len;       /// block size
    void* private_data;        /// block content
    short valid;              

} vfs_file_t;
```
For now, the file or device has only one block content.

#### Path Management
- Unlike Linux kernel which uses a dentry structure for file paths, this implementation uses a hashmap for simplicity since folder management is not the focus.

#### Hashmap Structure
```text
|filename| -> [file-0] -> [file-20] -> [inode-k] -> [inode-z]
|filename|
|filename| -> [file-1] -> [file-m]
```

```c
typedef struct vfs_sys_s {

    hash_map_t* vfs_files_map;   /// the hashmap above
    queue_t* vfs_files_recycle;  /// recycle queue
    arch_lock_t sys_lock;        /// system level lock.(not used)

} vfs_sys_t;
```

### Performance
- Utilizes DJB2 hash function for simplicity and speed.
- Common hashmap search is O(1) to O(n), but using a BTree behind each hash index node rather than a linked list can improve performance.

### Parallelism
- **Concurrent Creation/Deletion Safety:** Uses hash roots level `rw_lock` to minimize blocking cases.
- **File Descriptor (FD) Operations Safety:** File pointer address is the internal association for `open/close/read/write`, and `rw_lock` ensures concurrency safety.
- **Creation/Deletion vs. FD Operations Safety:** Deletion sets `vfs_file->valid = 0`, removes from hashmap, and recycles the pointer to prevent use of dirty pointers.

### Scalability
- Hashmap has fixed memory for root nodes and flexible memory for collision nodes.
- In real scenarios, kernel updates structures based on user hardware access invokes (e.g., `/proc/slabinfo` shows inode cache in the kernel).

### Maintainability
- Code is organized into three layers:
  - **Kernel:** Main subsystem for FD-file operations.
  - **Arch:** Linux-specific implementations (lock/task/panic/signal).
  - **Driver:** Actual device (file) operations under VFS.
  - **Lib:** Data structures and tools.

## 1.2 SYS
System call layer implementations for `sys_open/read/write/close`.

### Functions
```c
int sys_open(tsk_id_t tid, const char* path, unsigned int mode);
int sys_close(tsk_id_t tid, int fd);
int sys_read(tsk_id_t tid, int fd, char *buf, size_t len, unsigned long* pos);
int sys_seek(tsk_id_t tid, int fd, unsigned long pos);
int sys_write(tsk_id_t tid, int fd, const char *buf, size_t len, unsigned long pos);
```

### Significant Data Structure of fd to file map.
```text
Direct Table
Tasks       Buckets      Handles
|task1| -> |bucket1| -> [fd0] -> [fd1] -> ... -> [fdk]
           |bucket2| -> [fdk+1] -> .. -> [fdn]
              ...
|task2| -> |bucket1| -> [fd0]
           |bucket2| -> nil
```

Bucket Struct
```c
typedef struct vfs_handle_bucket_s {

    vfs_handle_t** handles;          /// major handle struct
    arch_rw_lock_t* handle_rw_locks; /// only when subtasks enabled

} vfs_handle_bucket_t;
```

Handle Struct
```c
typedef struct vfs_handle_s {

    int fd;
    unsigned long read_pos;
    unsigned long mode;
    unsigned long ptr_ptr_file_addr;   /// internal pointer address
    arch_lock_t read_pos_lock;         /// only used for pos update

} vfs_handle_t;
```

When tasks are isolated with each other, task-1 cannot directly access task-2's fd.
The first step of sys_open/read/write/close, it gets the current task pointer.

Normally, kernel will know the current task id immediately because kernel maintain the full context.
However, as our arch layer is linux, linux gets pthread_t as current thread_id which again, needs a hashmap to 
an actually index. This assignment simplified this part because this is not what is stressed. By passing the
task_id to the sys_open/read/write/close functions, it simulates the kernel getting the task_id.

## Performance

### Performance of fd search
- The 1st O(1), get task pointer from `task_it(tsk_id_t)`
- The 2nd O(1), get bucket by `i_bucket` being calculated by `fd`.
- The 3rd O(1), get handle by `i_handle` being calculated by `fd`.

```c
i_bucket = fd / ARCH_VFS_FDS_PER_BUCKET;
i_handle = fd % ARCH_VFS_FDS_PER_BUCKET;
```

Bucket size equals to the `MAX_FDS_NUM / BUCKET_NUM`

So overall the TC is O(1), the SC is O(n), n is the open fds.


### Performance of open/close operations

During open/close, new fd is created or fetched from fd cache queue. 
The queue fetch is O(1) and save it to the handles is also O(1), vice versa.

```c
Tasks       Buckets      Handles
|task1| -> |bucket1| -> [fd0] -> [fd1] -> ... -> [fdk]
           |bucket2| -> [fdk+1] -> .. -> [fdn]
              ...
|task2| -> |bucket1| -> [fd0] -> [fd3]
           |bucket2| -> nil      /
                                / O(1)
                               /
|task2|      fd cache queue  [fd3] -> [fd4] -> ... [fdm]
```

## Parallelism

NOTICE: 
Parallelism is only a concern when supporing sub-tasks. Because tasks are isolated, buckets and handles are also task unique.
But if there are sub-tasks sharing fds with each other, the parallelism is a concern.

### SUB-TASKS Parallelism

#### Question-1 
The sync during fd open.
Handles will be allocated lazily during fd open. 
The handles buffer allocation only races against itself, why?

When we use `calloc()`, the process can be below.
```c
ptr = calloc()
```
- execute calloc() pure function, it has no parallel issue at all.
- `ptr = __ptr`, the `__ptr` is the return value of `calloc()`. (pointer is `unsigned long`).
The memory content of `__ptr` has already been set to all 0 in `calloc()`.
On another thread, `if(ptr)` will return either `true` as `ptr` has value or `false` as `ptr == NULL`.


#### Solution of Question-1
```c
Tasks   sub-tasks   Buckets     Handles
task1
	|sub-task1| -> |bucket1| -> [fd0] -> [fd1] -> ... -> [fdk]
                       |bucket2| -> [fdk+1] -> .. -> [fdn]
		        ...
	|sub-task2| -> |bucket1| -> [fd0]
	               |bucket2<rw_lock>| -> nil
                                |
                              lock
```
Bucket level lock will make the handles in bucket be safe to be allocated parallelly.

Also, Double lookup logic is recommanded for a better performance.
```c
/// first lookup
if(bucket->handles == NULL)
    lock
        if(bucket->handles == NULL)
        /// second lookup.
    unlock
```

#### Question-2 The handle operations concurrency issue

In Question-1, the `bucket->handles` array's parallel issue is solved.
But when operating `bucket->handles[i]` parallelly will cause new parallel issues.

#### Solution of Question-2
Investing handle level rw_locks.

```c
Tasks  sub-tasks    Buckets      Handles        lock_w
task1                                           |
	|sub-task1| -> |bucket1| -> [fd0<rw_lock>] -> [fd1] -> ... -> [fdk]
                       |bucket2| -> [fdk+1] -> ..  -> [fdn]
		        ...
	|sub-task2| -> |bucket1| -> [fd0]
	               |bucket2| -> nil
```
When changing a handle's fields, `lock_w` the `rw_lock` on handle-lock level.
Will this cause fd search performance loss? No, why? 

- Reason-1: open function will use pure alloc function to alloc handle pointer, then set address to the reference in the table.

- Reason-2: fd search returns handle's pointer. During fd search, no handle level lock will be performed. 

```c
    /// Pure process start
    vfs_handle_t* ptr_handle = (vfs_handle_t*) malloc(sizeof(vfs_handle_t));
    ptr_handle->fd = nfd;
    ptr_handle->mode = mode;
    ptr_handle->read_pos = 0;
    ptr_handle->ptr_ptr_file_addr = file_ptr_addr;
    arch_spin_lock_init(&(ptr_handle->read_pos_lock));
    /// Pure process end

    /// handle w lock
    arch_rw_lock_w(&(tsk->ts_handle_buckets->handle_rw_locks[nfd]));

    /// give the value to table.
    ptr_bucket->handles[i_handle] = ptr_handle;

    /// handle w unlock
    arch_rw_unlock_w(&(tsk->ts_handle_buckets->handle_rw_locks[nfd]));
```
When searching fd, either `ptr_bucket->handles[i_handle]` is `NULL` or having value, then return the address simply.
Again, non-subtasks mode has no concern of "Parallelism" of handles access by task isolation.

Unlike handle search function, the `read/write/close` will need to perform handle `rw_lock` while doing handle updates, such as updating `read_pos`, releasing handle memory, etc.



### Parallelism Concern + Performance Concern

There is an extra memory cost when investing `handle_lock` and `bucket_lock`.
Coming to the practical cases, in development period,
handle structure will have more and more fields. However, the locks
are not the case, even if giving a maximum number of locks for all possible handles, it won't grow with handle fields design,
but will grow only with tasks number.

For one task, the Space cost is O(C), C is a const. The tasks number is t, then the space cost is O(C*t)

## 1.3 task

Task module is not a significant module for this assignment.
But providing a simple task system can well simulate the real scenario of a kernel.

```c
Task1     Task2    Task3    Task4 ..
|          |        |        |
|open/r/w  |        |        |
|          |        |        |
[   Kernel structure (vfs_handle, vfs_file...) ]
```

For now, each task is one pthread on linux. The tasks pointers are saved by `task_manager` in a fixed array.
Linux by default has maximum process number of 32768, means the maximum number of tasks is not really big. But sitll, the fixed initial memory cost can reach 200+KB.
Using a dynamic array will be much better. 

Because task module is not what this assignment focused on, it simply uses a fixed pointers' array which may consume some KBs during startup(20+KB for 2048 tasks).



# 2. Code & Project

## 2.1 Folders
```c
└── src
    ├── arch
    │   └── linux
    │       └── arch.c
    ├── include
    │   ├── arch
    │   │   └── linux
    │   │       └── arch.h
    │   ├── kernel
    │   │   ├── defs.h
    │   │   ├── sys.h
    │   │   ├── task.h
    │   │   └── vfs.h
    │   └── lib
    │       ├── atomic.h
    │       ├── hash.h
    │       └── queue.h
    ├── kernel
    │   ├── sys.c
    │   ├── task.c
    │   └── vfs.c
    ├── lib
    │   ├── hash.c
    │   └── queue.c
    └── tests
        ├── lib
        │   └── test_hash.c
        ├── test_def.h
        ├── test_fd_reuse_sanity.c
        ├── test_open_close_concurrency.c
        ├── test_r1w3_subtasks_1.c
        ├── test_r1w3_subtasks_2.c
        ├── test_rlock_concurrency_1.c
        ├── test_rwlock_concurrency_1.c
        ├── test_rwlock_concurrency_2.c
        ├── test_rw_sanity.c
        ├── tests.c
        ├── test_vfs_file_create.c
        └── test_vfs_file_search.c
```

All code files are in src, kernel folder has the major code.
- sys.h open/read/write/close definitions.
- vfs.h major file/handle structs
- task.h task related structs
- sys.c open/read/write/close 
- vfs.c file/handle 
- task.c task/task_manager 

## 2.2 Build & Tests
Call `make` under the project folder.

```shell
make
```
Then run main to go through main tests flow.

```shell
./main
```
Benchmark will be shown after all tests done.
```shell
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 35.34    1.199688         331      3620           mmap
 21.16    0.718290         199      3610           mprotect
 16.43    0.557619         303      1843           brk
 16.17    0.548928         152      3606           clone
 10.87    0.368868       16038        23           clock_nanosleep
  0.02    0.000717          29        25           write
  0.00    0.000066          17         4           futex
  0.00    0.000050          50         1           munmap
  0.00    0.000017           4         4           fstat
  0.00    0.000015           8         2           rt_sigaction
  0.00    0.000015          15         1           rt_sigprocmask
  0.00    0.000015           8         2         1 arch_prctl
  0.00    0.000015          15         1           prlimit64
  0.00    0.000000           0         2           read
  0.00    0.000000           0         3           close
  0.00    0.000000           0         8           pread64
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         3           openat
  0.00    0.000000           0         1           set_robust_list
------ ----------- ----------- --------- --------- ----------------
100.00    3.394303                 12762         2 total
```

Some concurrent tests will start 1000-2000 tasks, trustling tests include 2000 read&write per task.
See `test_rlock_concurrency_1.c::test_rlock_same_file_from_many_tasks`

## 2.3 Function Usages

```c
tsk_id_t tid;
int rt = task_create(&tid, task_execute_func);
```

In the `test_execute_func`
```c
void* task_execute_func(void* param)
{
    tsk_id_t tid = (tsk_id_t) param;
    ...
}
```
In the `test_execute_func` call open/read/write/close
```c
int fd = sys_open(tid, "/dev/devtest/same_block", RW_CREATE);
int pos;
...
sys_read(tid, fd, buf_read, siz, &pos);
...
sys_write(tid, fd, buf_write, siz, pos);
...
sys_close(tid, fd);
```

## 2.4. Which Is Not Good Enough
There are some parts not being finished perfectly.

- The task_destroy method sometimes trigger futex errors. I'm still trying to figure out.
    However, task management and arch layer are not what the assignment focuses on.

- Driver extension mode. I really like the driver dev block ioctl extension mode the linux kernel provided.
    However, this part needs extra design and effort. May not be done in this assignment.

- Reading and writing mode, blocking mode, non-blocking mode, sync mode, aync mode, etc.
    This is very complex and also related to multiplexing.

- System lifetime management. This system has not much lifespan runtime management being done.
    Ideally, the kernel threads play some important roles. 

# 3. Other Significant Concerns


## 3.1 Better data structure.

All directly indexed linear structure should be Dynamically Growing Array.
This will save the memory during intitial stages and the searching time cost is still low.

## 3.2 Kernel Threads Collect Memory(buckets/handles)
 
For now, there are 2 places calling calloc which might be slower than malloc of dirty.
However, in real scenarios, these memory will not be released very frequently. Once allocated, system should
keep it for re-usable cases. But this kinds of memory management system is very complex.It relates to modules and functionalities such as IPC/Signal. This assignment did not implement this part.

## 3.3 More Benchmark and Performance testing.

In actual projects, better benchmark and performance toolkits and libs must be developed or involved.
