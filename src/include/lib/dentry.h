#ifndef DENTRY_H
#define DENTRY_H

#include "hash.h"

/*
/<module-type>/<sub-type>/<name>

For example

/dev/cam/cam-0-0
 |     |      |
16 Hash|      |
       |      |
       64 Hash|
              |
             256 Hash

*/

#define ROOT_DENTRY_SIZE 16

typedef struct dentry_node_s {
    hash_map_t *hash_map;
} dentry_node_t;


dentry_node_t* dentry_node_create(int hash_bucket_size);
void dentry_node_insert(dentry_node_t* node, const char *key, unsigned long value);
void dentry_node_delete(dentry_node_t* node, const char *key);

#endif
