#ifndef INODE_H
#define INODE_H

typedef struct inode_s {
    unsigned short fd;
    unsigned long data_ptr_address;
} inode_t;


#endif
