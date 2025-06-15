#include "string.h"
#include "kv_store.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    kv_init(); // Initialize table
    
    put("usr", "Orvin");
    put("pswrd", "1234");
    put("apple", "red");         // Insert
    put("banana", "yellow");
    put("grape", "purple");
    put("apple", "juicy");

    print();

    char const* val = get("apple");
    printf("Value at apple: %s", val);

    kv_delete("apple");

    printf("\n");

    print();

    return 0;


}