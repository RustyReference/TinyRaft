#ifndef KV_STORE_H
#define KV_STORE_H

#include <pthread.h>

#define TABLE_SIZE 100
#define MAX_LINE_SIZE 1024
#define DB_FILENAME "db.txt"

typedef struct entry {
    char* key;
    char* value;
    struct entry* next;
} entry_t;

// Initialize the database (hash table + mutex + load from file)
void db_init();

// Clean up and save to file
void cleanup();

// Thread-safe operations
void put(const char* key, const char* value);
char* get(const char* key);
void delete_entry(const char* key);
void print();

// File operations (called automatically by put/delete)
int save_to_file();
int load_from_file();

#endif
