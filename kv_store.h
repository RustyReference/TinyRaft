#ifndef KV_STORE_H
#define KV_STORE_H

#define TABLE_SiZE 100

typedef struct entry {
    char* key;
    char* value;
    struct entry* next;
} entry_t;

// Initializes all table to null
void kv_init();

// Inserts a key-value pair into the hash table
// If the key already exists, it updates the value
void put(const char* key, const char* value);

char* get(const char* key);

void kv_delete(const char* key);

void print();

#endif
