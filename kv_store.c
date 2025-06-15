#include <stdio.h>     
#include <stdlib.h>    
#include <string.h>    
#include "kv_store.h"  

// Declaring hash table for 100 entries
static entry_t* table[10];

/* djb2 hash function
    @str : string to be hashed
    @RETURN: hash % 100, returns valid index for table
*/
unsigned long hash(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % 10;
}

// Initializes hash table with null entries
void kv_init() {
    for (int i = 0; i < 10; i++) {
        table[i] = NULL;
    }
}

// Inserts a key-value pair into the hash table
// If the key already exists, it updates the value
// @key : the key that maps given value
// @value: value to mapped 

void put(const char* key, const char* value) {
    unsigned long idx = hash(key);
    entry_t* head = table[idx];

    // Check if key exists
    while (head != NULL) {
        int n = 0;
        // Sets "n" to length of longer string
        if (strlen(head->key)<strlen(key)) {  // Compares length of key strings 
            n = strlen(key);                  
        }
        else {
            n = strlen(head->key);
        }
        if (strncmp(head->key, key, n) == 0) {
            free(head->value);                    // Clean up old value
            head->value = strdup(value);          // Store new one
            return;
        }
        head = head->next;
    }

    // Insert new entry at head
    entry_t* new_entry = (entry_t*)malloc(sizeof(entry_t));
    new_entry->key = strdup(key);                 // Make a copy of key
    new_entry->value = strdup(value);             // Make a copy of value
    new_entry->next = table[idx];                 // Point to previous head
    table[idx] = new_entry;                       // New entry becomes head
}

// Retrieves value at given key
// #RETURN : value at the key or NULL if key was not found
char* get(const char* key) {
    unsigned long idx = hash(key);
    entry_t* head = table[idx];
    while (head != NULL) {
        // Sets "n" to length of longer string
        int n = 0;
        if (strlen(head->key)<strlen(key)) {  // Compares length of key strings 
            n = strlen(key);                  
        }
        else {
            n = strlen(head->key);
        }
        if (strncmp(head->key, key, n) == 0) {
            return head->value;
        }
        head = head->next;
    }
    return NULL;
}

void kv_delete(char const* key) {
    unsigned long idx = hash(key);
    entry_t* cur = table[idx];
    entry_t* prev = NULL;
    // Search for key at idx
    while(cur != NULL) {
        // Sets "n" to length of longer string
        int n = 0;
        if (strlen(cur->key)<strlen(key)) {  // Compares length of key strings 
            n = strlen(key);                  
        }
        else {
            n = strlen(cur->key);
        }
        if (strncmp(cur->key, key, n)==0) {
            // Deleting from middle of list
            if(prev!=NULL) {
                prev->next = cur->next;  // Set previous to the next entry after current  
            }
            // Deleting from front 
            else if(prev==NULL) {
                table[idx] = cur->next;
            }
            // Free up data
            free(cur->key);
            free(cur->value);
            free(cur);
        }
        prev = cur;
        cur = cur->next;
    }
}

void print() {
    for(int i = 0; i<10; i++) {
        entry_t* head = table[i];
        if(head != NULL) {
            printf("[%d]: ", i);
            while(head != NULL) {
                printf("(%s : %s) ->", head->key, head->value);
                head = head->next;
            }

            printf("NULL\n");
        }
    }
}


