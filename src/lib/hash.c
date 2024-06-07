#include "hash.h"
#include <stdlib.h>
#include <string.h>

// Initialize the hash map
hash_map_t* hash_map_create(size_t bucket_count) {
    hash_map_t *map = (hash_map_t *)malloc(sizeof(hash_map_t));
    map->bucket_count = bucket_count;
    map->buckets = (hash_map_entry_t **)calloc(bucket_count, sizeof(hash_map_entry_t *));
    return map;
}

// Insert a key-value pair into the hash map
void hash_map_insert(hash_map_t *map, const char *key, unsigned long value) {
    unsigned long index = hash_function(key, map->bucket_count);
    hash_map_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    entry = (hash_map_entry_t *)malloc(sizeof(hash_map_entry_t));
    entry->key = key;
    entry->value = value;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;
}

int hash_map_get(hash_map_t *map, const char *key, unsigned long *value) {
    unsigned long index = hash_function(key, map->bucket_count);
    hash_map_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            *value = entry->value;
            return 1; // Found
        }
        entry = entry->next;
    }
    return 0; // Not found
}

int hash_map_remove(hash_map_t *map, const char *key) {
    unsigned long index = hash_function(key, map->bucket_count);
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
            return 0; // Removed
        }
        prev = entry;
        entry = entry->next;
    }
    return -1; // Not found
}

void free_hash_map(hash_map_t *map) {
    for (size_t i = 0; i < map->bucket_count; i++) {
        hash_map_entry_t *entry = map->buckets[i];
        while (entry) {
            hash_map_entry_t *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(map->buckets);
    free(map);
}
