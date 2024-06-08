#include "hash.h"
#include "arch.h"
#include <stdlib.h>
#include <string.h>

hash_map_t* hash_map_create(size_t bucket_count) {
    unsigned long idx;
    unsigned long siz = sizeof(hash_map_entry_t*);
    hash_map_t *map = (hash_map_t*) malloc(sizeof(hash_map_t));
    map->bucket_count = bucket_count;
    map->buckets = (hash_map_entry_t**) calloc(bucket_count, siz);
    if(map->buckets == NULL) {
        free(map->buckets);
        free(map);
        return NULL;
    }
    map->key_rw_locks = (arch_rw_lock_t*) calloc(bucket_count, sizeof(arch_rw_lock_t));
    for(idx = 0; idx < bucket_count; idx ++) {
        arch_rw_lock_init(&(map->key_rw_locks[idx]));
    }
    return map;
}

unsigned long hash_map_insert(hash_map_t *map, const char *key, unsigned long value) {
    unsigned long index = hash_function(key, map->bucket_count);

    arch_rw_lock_w(&(map->key_rw_locks[index]));
    hash_map_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;

            arch_rw_unlock_w(&(map->key_rw_locks[index]));
            return (unsigned long)(&(entry->value));
        }
        entry = entry->next;
    }
    entry = (hash_map_entry_t *)malloc(sizeof(hash_map_entry_t));
    entry->key = key;
    entry->value = value;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;

    arch_rw_unlock_w(&(map->key_rw_locks[index]));
    return (unsigned long)(&(entry->value));
}

int hash_map_get(hash_map_t *map, const char *key, unsigned long* value) {
    unsigned long index = hash_function(key, map->bucket_count);

    arch_rw_lock_r(&(map->key_rw_locks[index]));
    hash_map_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            *value = (unsigned long)(&(entry->value));

            arch_rw_unlock_r(&(map->key_rw_locks[index]));
            return 0;
        }
        entry = entry->next;
    }

    arch_rw_unlock_r(&(map->key_rw_locks[index]));
    return -1;
}

int hash_map_remove(hash_map_t *map, const char *key) {
    unsigned long index = hash_function(key, map->bucket_count);

    arch_rw_lock_w(&(map->key_rw_locks[index]));
    hash_map_entry_t *entry = map->buckets[index];
    hash_map_entry_t *prev = NULL;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            free(entry);

            arch_rw_unlock_w(&(map->key_rw_locks[index]));
            return 0; // Removed
        }
        prev = entry;
        entry = entry->next;
    }

    arch_rw_unlock_w(&(map->key_rw_locks[index]));
    return -1; // Not found
}

void hash_map_dump(hash_map_t *map, int (*dump_hook)(unsigned long, const char*, unsigned long)) {
    unsigned long i = 0;
    hash_map_entry_t *entry = NULL;

    for(; i < map->bucket_count; i ++) {
        entry = map->buckets[i];
        while (entry) {
            if(!dump_hook) {
                printf("%p -> ", (void*)(entry->value));
            }
            else {
                if(dump_hook(i, entry->key, entry->value)) {
                    return;
                }
            }
            entry = entry->next;
        }
    }
}
