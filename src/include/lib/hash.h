#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include "arch.h"

#define HASH_VALUE_TO_PTR(v, type) ((type*)v)

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL

// Define the structure for a hash map entry
typedef struct hash_map_entry_s {
    const char *key;
    unsigned long value;
    struct hash_map_entry_s *next;
} hash_map_entry_t;

// Define the structure for the hash map
typedef struct {
    hash_map_entry_t** buckets;
    size_t bucket_count;
    arch_rw_lock_t* key_rw_locks;
} hash_map_t;

// Hash function for string keys
static __inline__ unsigned long hash_function(const char *key, size_t bucket_count) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash % bucket_count;
}

hash_map_t* hash_map_create(size_t bucket_count);
unsigned long hash_map_insert(hash_map_t *map, const char *key, unsigned long value);
int hash_map_get(hash_map_t *map, const char *key, unsigned long* value);
int hash_map_remove(hash_map_t *map, const char *key);
void hash_map_dump(hash_map_t *map, int (*dump_hook)(unsigned long idx, const char*, unsigned long));

#endif
