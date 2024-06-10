1. Major modules
1.1 vfs
Virtual File System Structure for path file search.

In the code, file is vfs_file_t struct. 
It is in global scope which is cross-task accessible.
The major concern of vfs_file_t is designed to be relatively effecient but concurrently safe.

Though the scalability of linux-like ko driver extension is a good manor, but here has no driver extension implementation 
and we will discuss the design idea of it.

Though mostly of the versions of Linux kernel use dentry structure. 
Dentry organizes the file paths more like a tree. As our assignment does not focus on folder management and search,
by using a hashmap can simplify the problem.

Hashmap
|filename| -> [file-0] -> [file-20] -> [inode-k] -> [inode-z]
|filename|
|filename| -> [file-1] -> [file-m]

Performance Concern.
Besides the DJB2 hash function is simple and fast, the common hashmap search is O(1) to O(n).
A better idea is to use a BTree behind each hash index node rather than a link-list.

Hashmap
|filename| -> [file-0] -> [file-20] -> [inode-k] -> [inode-z]
|filename|	 \			\
|filename|    [file-1] -> [file-m]	[inode-k-1]


The search operation only related to file creation/deletion and fd open which is sys_open in this assignment.
sys_read/sys_write/sys_close won't need to touch path-to-file map. Therefore, this assignment uses link-list for a simple implementation.

Steps
(1) When to create a file, generate hash-key based on filename(fullpath).
(2) Do a O(1) best, O(n) worst hash search and find the vfs_file_t pointer address.
(3) Using pointer's pointer(address of a pointer) is convenient for global reference.


Parallelism Concern.

File access can be cross-task, here are 2 types of questions.
Question-1: How to keep concurrent creation/deletion safe?
Question-2: How to keep safe among fd open/close/read/write?
Question-3: How to keep safe between file creation/deletion and fd open/close/read/write?

Question-1 How to keep concurrent creation/deletion safe?
By using hash roots level rw_lock, the blocking cases can be minimized.
  
 Root Hash nodes
|filename1<rw_lock1>| -> [file-0] -> [file-20] -> [inode-k] -> [inode-z]
|filename2<rw_lock2>|
|filename3<rw_lock3>| -> [file-1] -> [file-m]

To concurrently update a data structure, we need to minimized the scope of synchronize block.
An example is a BTree, we lock the target node and target's parent node to make brother tree branch concurrent.

The same idea here, we lock a hash root node to make other hash roots concurrent.
For example,

These 3 files have a same hash key.
"/dev/usb/usb_cam0"
"/dev/usb/usb_keyboard0"
"/dev/usb/usb_trunk1"

If any of them goes an update, it locks w_lock. Readings will trigger r_lock, so will be parallel between readings.

                      RW_LOCK
                         |
|"/dev/usb/usb_cam0"<rw_lock1>| -> ["/dev/usb/usb_keyboard0"] -> ["/dev/usb/usb_trunk1"]
|filename2<rw_lock2>|
|filename3<rw_lock3>| -> [file-1] -> [file-m]

Question-1's schedule graph

[vfs_file_get_or_create]              [vfs_file_delete]
          |                       	     |
     file_search[1]                     file_search
          |                                  |
       found                              found
          |                                  |
        return                           lock_file_w
                                           update
                                         unlock_file_w
                                             |
                                         hash_lock[1]
					   remove[1]
					 hash_unlock[1]
                                             |
                                           queue
                                             |
                                           return

file_search[1] and hash_lock[1] have sync.


Question-2 How to keep safe among fd open/close/read/write?

open/close/read/write are fd level operations, the file pointer address the only internal association for these operations.
These functions belong to sys.c and will not touch any file operations directly.
But when it comes to the actual read/write/open/close, the file must be modified. 
Then sys call to vfs and trigger file operations, the file level rw_lock must be assigned.

                      sys_read()                sys_write()
                             \                   /
                            vfs_read()   vfs_write()
                                  \      /
                                  RW_LOCK    
                                    |
|filename1<rw_lock1>| -> [file-0<f_rw_lock>] -> [file-20<f_rw_lock>] -> [inode-k<f_rw_lock>] -> [inode-z<f_rw_lock>]
|filename2<rw_lock2>|
|filename3<rw_lock3>| -> [file-1] -> [file-m]

Among open/close/read/write, the content of the file pointer address won't be touched, 
open/close are only association operations between fd and file. So file level rw_lock will make the concurrency safe.


Question-3 How to keep safe between file creation/deletion and fd open/close/read/write?

When open/close/read/write only use file pointer address as reference, also these file pointers are isolated and managed in vfs_sys.
new file creation will not impact the existing file pointers. 

File deletion will cause open/read/write/close to use a dirty file pointer. The solution is, never free the file pointer during vfs_file_delete.
During a file deletion,
set the vfs_file->valid = 0 at first pace, 
then move the vfs_file* out of the hashmap, 
free the hashnode, free the vfs_file's content data(the largest part of memory)
save the vfs_file pointer to vfs_files_recycle queue.
Later on, the kernel thread will loop the recycle queue vfs_files_recycle and free it with a system level only when its reference count == 0.

Here has a balancing thinking, we need to balance the parallel safety performance cost and memory occupation cost.
For now, the decision was, release the actual file data immediately during file deletion. 
Another idea is, let kernel thread free the actual vfs_file's content data later.

                      sys_read()                
                             \                  
                            vfs_read()   [vfs_delete_file only during file data release]
                                  \      /
                                  RW_LOCK    
                                    |
|filename1<rw_lock1>| -> [file-0<f_rw_lock>] -> [file-20<f_rw_lock>] -> [inode-k<f_rw_lock>] -> [inode-z<f_rw_lock>]
|filename2<rw_lock2>|
|filename3<rw_lock3>| -> [file-1] -> [file-m]

Question-3's schedule graph

[vfs_read/write]                    [vfs_file_delete]
          |                       	     |
       file_by_fd                       file_search
          |                                  |
       lock_file_r/w                         |
      actual r/w[1]                          |
       unlock_file_r/w                     found
          |                                  |
        return                           lock_file_w
                                           update[1]
                                         unlock_file_w
                                             |
                                         hash_lock
					   remove
					 hash_unlock
                                             |
                                           queue
                                             |
                                           return

The actual r/w[1] and update[1] have sync lock.

Parallelism + Performance Concern.
Even if the Hashmap structure performance is acceptable, the parallel sync locks may lower the overall performance.
The overall solutions are,
*By the isolated file pointer design, different files operations won't block each other at all.
*By the hash roots level rw_lock, the management related file nodes operations blocking will be minimized.
*By file recycle queue, file deletions won't race with fd association but will race with actual vfs_write/vfs_read.


Scalability Concern.
The Hashmap has fixed memory for root nodes and flexible memory expansion for collisions nodes.
Because this assignment does not focus on hardware adaptation nor real files management.
In real scenarios, kernel like linux must update the structure according to user hardware access invokes.
For example the /proc/slabinfo shows the inode cache in the kernel. This won't be the max files number.

This assignment didn't implement the driver extension pattern but as a concern, hooking driver functions
during file create/read/write/delete will be a good idea. Using ioctl general methods implementations on user driver side.


Maintainability Concern.

The code is designed as 3 layers. 
kernel is general and doing the main purpose of the sub-system fd-file works.
arch means linux implementation when on linux, including lock/task(thread or proc)/panic/signal, etc.
driver means the actual device(file) operation under vfs.
lib are data structure and tools for all to use.
[  kernel    ]
[arch][driver]
[   lib      ]

This assignment implemented part of the arch layer to support task and subtask, panic, lock.
Driver module is not implemented.
lib includes hash/queue


1.2 sys

System call layer, sys_open/read/write/close implementations

int sys_open(tsk_id_t tid, const char* path, unsigned int mode);
int sys_close(tsk_id_t tid, int fd);
int sys_read(tsk_id_t tid, int fd, char *buf, size_t len, unsigned long* pos);
int sys_seek(tsk_id_t tid, int fd, unsigned long pos);
int sys_write(tsk_id_t tid, int fd, const char *buf, size_t len, unsigned long pos);

Significant Data Structure for fd<->file map.

Direct Table
Tasks       Buckets      Handles
|task1| -> |bucket1| -> [fd0] -> [fd1] -> ... -> [fdk]
           |bucket2| -> [fdk+1] -> .. -> [fdn]
              ...
|task2| -> |bucket1| -> [fd0]
           |bucket2| -> nil


When tasks are isolated with each other, task-1 cannot directly access task-2's fd.
The first step of sys_open/read/write/close, it gets the current task pointer.

Normally, kernel will know the current task id immediately because kernel maintain the full context.
However, as our arch layer is linux, linux gets pthread_t as current thread_id which again, needs a hashmap to 
an actually index. This assignment simplified this part because this is not what is stressed. By passing the
task_id to the sys_open/read/write/close functions, it simulates the kernel getting the task_id.

Performance Concern.

Performance of fd search:
The 1st O(1), get task pointer from task_it(tsk_id_t)
The 2nd O(1), get bucket by i_bucket being calculated by fd.
The 3rd O(1), get handle by i_handle being calculated by fd.

i_bucket = fd / ARCH_VFS_FDS_PER_BUCKET;
i_handle = fd % ARCH_VFS_FDS_PER_BUCKET;

Bucket has a size, it equals to the MAX_FDS_NUM / BUCKET_NUM

So overall the TC is O(1), the SC is O(n), n is the open fds.

Performance of open/close operations

During open/close, new fd is created or fetched from fd cache queue. 
The queue fetch is O(1) and save it to the handles is also O(1), vice versa.

Tasks       Buckets      Handles
|task1| -> |bucket1| -> [fd0] -> [fd1] -> ... -> [fdk]
           |bucket2| -> [fdk+1] -> .. -> [fdn]
              ...
|task2| -> |bucket1| -> [fd0] -> [fd3]
           |bucket2| -> nil      /
                                / O(1)
                               /
|task2|      fd cache queue  [fd3] -> [fd4] -> ... [fdm]


Parallelism Concern.

NOTICE: 
Parallelism is only a concern when supporing sub-tasks. Because tasks are isolated, buckets and handles are also task unique.
But if there are sub-tasks sharing fds with each other, the parallelism is a concern.

SUB-TASKS scenario parallelism design.

Question-1 The sync during fd open.
Handles will be allocated lazily during fd open. 
The handles buffer allocation only races against itself, why?

When we use calloc(), the process can be below.
ptr = calloc()

1st step, execute calloc() pure function, it has no parallel issue at all.
2nd step, ptr = __ptr, the __ptr is the return value of calloc(). (pointer is also unsigned long).
The memory content of __ptr has already been set to all 0 in calloc().
On another thread, if(ptr) will return either true as ptr has value. 


Solution for "The handles buffer allocation only races against itself"
Tasks       Buckets      Handles
|task1|
	|sub-task1| -> |bucket1| -> [fd0] -> [fd1] -> ... -> [fdk]
		       |bucket2| -> [fdk+1] -> .. -> [fdn]
		        ...
	|sub-task2| -> |bucket1| -> [fd0]
		       |bucket2<rw_lock>| -> nil
                                |
                              lock
Bucket level lock will make the handles in bucket be safe to be allocated parallelly.

Double lookup logic is recommanded for a better performance.

/// first lookup
if(bucket->handles == NULL)
    lock
        if(bucket->handles == NULL)
        /// second lookup.
    unlock


Question-2 The handle operations concurrency issue

In Question-1, the bucket->handles array's parallel issue is solved.
But when operating bucket->handles[i] parallelly will cause new parallel issues.

Solution of Question-2

Investing handle level rw_locks.

Tasks       Buckets      Handles         lock_w
|task1|                                    |
	|sub-task1| -> |bucket1| -> [fd0<rw_lock>] -> [fd1] -> ... -> [fdk]
		       |bucket2| -> [fdk+1] -> .. -> [fdn]
		        ...
	|sub-task2| -> |bucket1| -> [fd0]
		       |bucket2| -> nil

When changing a handle's fields, lock_w the rw_lock on handle-lock level.
Will this cause fd search performance loss? No, why? 
Reason-1: fd search only return handle pointer. The read/write/close will perform handle rw_lock when doing handle updates
such as read_pos, or even releasing this handle memory and point to NULL.

Reason-2: open function will use pure alloc function to alloc local pointer, 
then give address to the handle pointer in the table.

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

When searching fd, either ptr_bucket->handles[i_handle] is NULL or having value, then return the address simply.

Again, non-subtasks mode has no concern about "Parallelism" for handle operations.


Parallelism Concern + Performance Concern

There indeed an extra memory cost when there are handle_lock and bucket_lock.
When coming to the practical cases, the handle structure must be scalable and extendable.
Handle structure will have more and more fields durding the system development. However, the locks
are not the case, even if giving a maximum number of locks for all possible handles, it won't grow with handle fields design,
but will grow only with tasks number. 

For one task, the Space cost is O(C), C is a const. The tasks number is t, then the space cost is O(C*t)


1.3 task
Task module is not a significant module for this assignment.
But providing a simple task system can well simulate the real scenario of a kernel.

Task1     Task2    Task3    Task4 ..
|          |        |        |
|open/r/w  |        |        |
|          |        |        |
[   Kernel structure (vfs_handle, vfs_file...) ]


For now, each task is one pthread on linux. The tasks are m









