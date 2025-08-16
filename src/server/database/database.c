#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "../server.h"
#include "database.h"

// Global hash table and mutex
static entry_t* table[TABLE_SIZE];
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

/* djb2 hash function
    @str : string to be hashed
    @RETURN: hash % 100, returns valid index for table
*/

unsigned long hash(const char* str) {
    	unsigned long hash = 5381;
    	int c;
    	while ((c = *str++)) {
        	hash = ((hash << 5) + hash) + c;
	}	
    	return hash % TABLE_SIZE;
}

// Initialize database
void db_init() {
    	// Lock during initialization
    	pthread_mutex_lock(&db_mutex);
    
    	// Clear hash table
    	for (int i = 0; i < TABLE_SIZE; i++) {
        	table[i] = NULL;
    	}
    
    	// Load existing data from file
    	load_from_file();
    
    	pthread_mutex_unlock(&db_mutex);
    	printf("Database initialized\n");
}

// Clean up - call when shutting down to free data
void cleanup() {
    	pthread_mutex_lock(&db_mutex);
    
    	// Save current state to file
    	save_to_file();
    
    	// Free all entries
    	for (int i = 0; i < TABLE_SIZE; i++) {
        	entry_t* head = table[i];
        	while (head != NULL) {
            		entry_t* temp = head;
            		head = head->next;
            		free(temp->key);
            		free(temp->value);
            		free(temp);
        	}
        	table[i] = NULL;
    	}
    
    	pthread_mutex_unlock(&db_mutex);
    	printf("Database cleaned up\n");
}

// Append key:value with thread safety
void put(const char* key, const char* value) {
    	if (!key || !value) return;
    
    	pthread_mutex_lock(&db_mutex);
    
    	unsigned long idx = hash(key);
    	entry_t* head = table[idx];
    
    	// Check if key exists (update case)
    	while (head != NULL) {
        	if (strcmp(head->key, key) == 0) {
            		free(head->value);
            		head->value = strdup(value);
            		save_to_file();  // Update file
			printf("save_to_file() returned:");
            		pthread_mutex_unlock(&db_mutex);  
            		printf("Updated: %s = %s\n", key, value);
            		return;
        	}
        	head = head->next;
    	}
    
    	// Insert new entry (new key)
    	entry_t* new_entry = (entry_t*)malloc(sizeof(entry_t));
    	if (!new_entry) {
        	pthread_mutex_unlock(&db_mutex);  // Unlock on error
        	return;
    	}
    
    	new_entry->key = strdup(key);
    	new_entry->value = strdup(value);
    	new_entry->next = table[idx];
    	table[idx] = new_entry;
    
    	save_to_file();  // Update file
    	pthread_mutex_unlock(&db_mutex); 
    
    	printf("Inserted: %s = %s\n", key, value);
}

// Get value at key 
char* get(const char* key) {
    	if (!key) return NULL;
    
    	pthread_mutex_lock(&db_mutex);
    
    	unsigned long idx = hash(key);
    	entry_t* head = table[idx];
    
    	while (head != NULL) {
        	if (strcmp(head->key, key) == 0) {
            		// Make a copy to return (caller should free it)
            		char* result = strdup(head->value);
            		pthread_mutex_unlock(&db_mutex);
            		return result;
        	}
        	head = head->next;
    	}
    
    	pthread_mutex_unlock(&db_mutex);
    	return NULL;
}

// Deletes entry from table and updates database file
void delete_entry(const char* key) {
    	if (!key) return;
    
    	pthread_mutex_lock(&db_mutex);
    
    	unsigned long idx = hash(key);
    	entry_t* cur = table[idx];
    	entry_t* prev = NULL;
    
    	while (cur != NULL) {
        	if (strcmp(cur->key, key) == 0) {
            		// Remove from linked list
            		if (prev != NULL) {
                		prev->next = cur->next;
            		} 
			else {
                		table[idx] = cur->next;
            		}
            
            		printf("Deleted: %s = %s\n", cur->key, cur->value);
            
            		// Free memory
            		free(cur->key);
            		free(cur->value);
            		free(cur);
            
            		save_to_file();  // Update file
            		pthread_mutex_unlock(&db_mutex);
            		return;
        	}
        	prev = cur;
        	cur = cur->next;
    	}
    
    	pthread_mutex_unlock(&db_mutex);
    	printf("Key not found for deletion: %s\n", key);
}

// Prints contents of hashtable
void print() {
	pthread_mutex_lock(&db_mutex);
    
    	printf("\n||Database Contents||\n");
    	int total_entries = 0;
    	for (int i = 0; i < TABLE_SIZE; i++) {
        	entry_t* head = table[i];
        	if (head != NULL) {
            		printf("[%d]: ", i);
            		while (head != NULL) {
                		printf("(%s : %s) -> ", head->key, head->value);
                		head = head->next;
                		total_entries++;
            		}
            		printf("NULL\n");
        	}
    	}
    
    	printf("Total entries: %d\n", total_entries);
    
    	pthread_mutex_unlock(&db_mutex);
}

// Save hash table to file - format: hash_index key value
int save_to_file() {

	FILE* file = fopen(DB_FILENAME, "w");
    	if (!file) {
        	printf("Error: Cannot open file for writing: %s\n", strerror(errno));
        	return -1;
    	}
    
    	for (int i = 0; i < TABLE_SIZE; i++) {
        	entry_t* head = table[i];
        	while (head != NULL) {
            		fprintf(file, "%d %s %s\n", i, head->key, head->value);
            		head = head->next;
        	}
    	}
   	 
    	fclose(file);

    	return 0;
}

// Load hash table from file
int load_from_file() {
    	FILE* file = fopen(DB_FILENAME, "r");
    	if (!file) {
        	printf("No existing database file found, starting fresh\n");
        	return 0;  // Not an error, just means no previous data
    	}
    
    	char line[MAX_LINE_SIZE];
    	int loaded_count = 0;
    
    	while (fgets(line, sizeof(line), file)) {
        	// Remove newline
        	line[strcspn(line, "\n")] = '\0';
        
        	// Parse: hash_index key value
        	/*char* hash_str = strtok(line, " ");
        	char* key = strtok(NULL, " ");
        	char* value = strtok(NULL, " ");*/
        	char* buf[255];
		int len = strnsplit(line, 1024, ' ', buf);
		if(len <= 0) {
			printf("ERROR failed to parse command\n");
			return -1;
		}

		char* key = buf[1]; // Key is second string in the line
		char* value = buf[2]; // Value is third string in the line
        	if (key && value) {
            		// Don't use put() here to avoid recursive file saves
            		unsigned long idx = hash(key);
            		entry_t* new_entry = (entry_t*)malloc(sizeof(entry_t));
            		new_entry->key = strdup(key);
            		new_entry->value = strdup(value);
            		new_entry->next = table[idx];
            		table[idx] = new_entry;
            
            		loaded_count++;
        	}
    	}
    
    	fclose(file);
    	printf("Loaded %d entries from database file\n", loaded_count);
    	return loaded_count;
}
