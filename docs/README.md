===================================
Task Running Model
===================================
Using linux threads to represent sub-os Tasks.

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
          Therefore, only the tasks sharing same inodes may be blocked by each other.

===================================
Major Data Structure
===================================

------------------------------------
fd(handle) to inode search.
------------------------------------
Two-Dimension hash table keys

Task-ID    fds

|Task1| -> [0] -> [1] -> [2] ... -> [k]
|Task2|
|Task3|
...
|TaskN|


Convert Thread-Id to hash-key:
Use hash function in /lib/hash.c


Two-Dimension hash table verlus

|Hash1, Queue1| -> [inode-0] -> [inode-1] -> [inode-k] -> [inode-z]
|Hash2, Queue2|                /               /
|Hash3, Queue3| ->      [inode-1] ... -> [inode-k]
...
|HashN, QueueN|

HashN is the hashtable storing <fd, inode*>
QueueN is the queue buffering the recycled fds.

Allocate a new fd
Step-1: Given thread-id and fd, find the 1-D hashtable node and pop its queue.
Step-2: If queue is empty, call next_fd() function to get a new fd, then save to 2-D hashtable of the current 1-D node.
Step-3: return the fd to user task.


inode search Time Complexity is O(C). C is a small const.
inode search Space Complexity is O(n). n is tasks and its open fds.




--------------------------------------
dev path to inode search.
--------------------------------------
/<module-type>/<sub-type>/<name>

For example

/dev/cam/cam-0-0
 |     |      |
16 Hash|      |
       |      |
       32 Hash|
              |
             128 Hash

dev: 16 hash buckets
cam: 32 hash buckets
cam-0-0: 128 hash buckets

Total support 65536 fds.
Linux by default supports 1619708 fds of all kinds.



