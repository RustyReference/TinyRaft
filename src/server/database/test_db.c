#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "database.h"

// Test function that multiple threads will run
void* thread_test(void* arg) {
    	int thread_id = *(int*)arg;
    	char key[50], value[50];
    
    	// Each thread inserts some data
    	for (int i = 0; i < 3; i++) {
        	sprintf(key, "thread%d_key%d", thread_id, i);
        	sprintf(value, "thread%d_value%d", thread_id, i);
        
        	printf("Thread %d: Inserting %s = %s\n", thread_id, key, value);
        	put(key, value);
        
       		// usleep(100000); // Sleep 0.1 seconds
    	}
    
    	return NULL;
}

int main() {
    	printf("|| Database Module Test ||\n");
    
    	// Initialize database
    	printf("\n1. Initializing database\n");
    	db_init();
    
    	// Test basic operations
    	printf("\n2. Testing basic operations\n");
    	put("name", "John");
    	put("age", "25");
    	put("city", "Dallas");
    
    	// Test retrieval
    	char* name = get("name");
    	char* age = get("age");
    	printf("Retrieved: name=%s, age=%s\n", name ? name : "NULL", age ? age : "NULL");
    	free(name);  // Remember to free!
    	free(age);
    
    	print();
    
    	// Test threading
    	printf("\nTesting with multiple threads\n");
    	pthread_t threads[3];
    	int thread_ids[3] = {1, 2, 3};
    
    	// Create 3 threads that all modify the database
    	for (int i = 0; i < 3; i++) {
        	pthread_create(&threads[i], NULL, thread_test, &thread_ids[i]);
    	}
    
    	// Wait for all threads to complete
    	for (int i = 0; i < 3; i++) {
        	pthread_join(threads[i], NULL);
    	}
    
    	print();
    
    	// Test deletion
    	printf("\nTesting deletion\n");
    	delete_entry("name");
    	delete_entry("thread1_key0");
    	print();
    
    	// Test cleanup
    	printf("\nCleaning up\n");
    	cleanup();
    
    	printf("\nTest completed! Check db.txt file.\n");
    	return 0;
}
