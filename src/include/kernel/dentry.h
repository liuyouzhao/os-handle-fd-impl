#ifndef DENTRY_H
#define DENTRY_H

#include "atomic.h"
#include "list.h"
#include "inode.h"

#define ROOT_DENTRY_SIZE 16

#define PTR_DEN(l) ((dentry_t*)l)

typedef struct dentry_s {
    atomic_t d_count;               // Reference count
    inode_t *d_inode;          // Associated inode
    struct dentry_s *d_parent;      // Parent directory
    const char* d_name;             // Name of the entry
    list_node_t* d_subdirs;         // List of subdirectories
} dentry_t;

dentry_t* dentry_create(const char* path, dentry_t* root);
dentry_t* dentry_delete(const char* path, dentry_t* root);
dentry_t* dentry_find(const char* path, dentry_t* root);

#endif
