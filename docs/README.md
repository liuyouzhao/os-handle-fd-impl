===================================
1. Task Running Model
===================================
Using linux threads to represent sub-os Tasks.
Thread-Id is Task-Id

Task1     Task2    Task3    Task4 ..
|          |        |        |
|open/r/w  |        |        |
|          |        |        |
[ Kernel Data structure (KDS)   ]

Do we need a kernel thread to operate KDS?
No, reasons are here.
Reason-1: KDS is not visible nor accessible to user tasks.(Assuming we have mem protection mode)
Reason-2: Each task operates its own 2-D hash table, no cross-access.
Reason-3: When a task accesses an inode, it locks the inode by using inode->i_sem signal for sync. 
          Therefore, only the tasks sharing same inodes may be blocked by each other, this is a common design for os.





===================================
2. Major Data Structure
===================================

------------------------------------
2.1 fd(handle) to inode
------------------------------------
2D-HASH

    <task-id, task_struct*>
                |
               task_struct::<fd, inode*>
                     

|Task1| -> [fd-0] -> [fd-1] -> [fd-2] ... -> [fd-k]
|Task2|
|Task3|
...
|TaskN|


Convert Task-Id to hash-key:
Use hash function in /lib/hash.c


Two-Dimension hash table values

|Hash1<fd, inode*>, Queue1<inode*>| -> [inode-0] -> [inode-1] -> [inode-k] -> [inode-z]
|Hash2<fd, inode*>, Queue2<inode*>|                /               /
|Hash3<fd, inode*>, Queue3<inode*>| ->        [inode-1] ... -> [inode-k]
...


## Open a new fd
Step-1: Given task-id and fd, find the 1-D hashtable node and pop its queue.
Step-2: If queue is empty, call next_fd() function to get a new fd, then save to 2-D hashtable of the current 1-D node.
Step-3: return the fd to user task.

## (Internal) Search inode by fd
Step-1: Given task-id and fd, find Task hash node, then use fd to get inode from <fd, inode*> hash


## Read by fd
Step-1: down_read()
Step-2: Find by search.
Step-3: up_read()


## Write by fd
Step-1: down_write()
Step-2: Search inode and write memory(memcpy).
Step-3: up_write()


## Close a fd
Step-1: down_close()
Step-2: Find the inode by the "Search". 
Step-3: Save the inode* to Queue<inode*>, set the hash inode* to NULL
Step-4: Return Queue's inode->fd to user task.
Step-5: up_close()



inode search Time Complexity is O(C). C is a small const.
inode search Space Complexity is O(n). n nearly equals to open fds.


--------------------------------------
2.2 path to inode* 
--------------------------------------
/<module-type>/<sub-type>/<name>

For example

/dev/cam/cam-0-0
 |     |      |
16 Hash|      |
       |      |
       64 Hash|
              |
             256 Hash

dev: 16 hash buckets
cam: 64 hash buckets
cam-0-0: 256 hash buckets


## Open
Step-1: create <module-type> in 16 hash buckets if it is not exist.
Step-2: create <sub-type> in 32 hash buckets if it is not exist.
Step-3: create <name> in 128 hash buckets if it is not exist.
Step-4: If the whole path exists, get the inode* and create fd and hash node by docs 2.1

## Delete a path
Step-1: down_delete()
Step-2: delete/free the inode
Step-3: up_delete()

## R/W and Close will use fd only. Pleasee see 2.1

Time Complexity is O(C) C is a small const.
Space Complexity is O(n) n nearly equals to open fds.



