#ifndef LLIST_H
#define LLIST_H

#include "../include.h"

// node
// @next : next node
// @prev : previous node
// @data : info it holds
struct node {
	struct node* next;
	void* data;
};

// linked list
// @first : head/start node
// @last  : tail/end node
struct llist {
	struct node* first;
	struct node* last;
};

// initialize a linked list. 
// #RETURN : new linked list.
struct llist llistCreat();

// add data to a linked list
// @list : list to add to
// @data : data to add
// #RETURN : status
// 	0 on error
// 	1 on success
int llistAdd(struct llist* list, void* data);

// insert data to a linked list's index
// @list  : list to add to
// @index : exact index you want data to end up in
// @data  : info to add
// #RETURN : status
// 	0 on error
// 	1 on success
int llistInsert(struct llist* list, int index, void* data);

// get data from a linked list
// @list  : list to get from
// @index : exact index you want to get
// #RETURN : pointer
// 	NULL on error
// 	a pointer to the value on success
void* llistGet(struct llist* list, int index);

// remove data from a linked list's index
// @list  : list to remove from
// @index : exact index you want gone. Memory will be freed. Existence will be forgotten. Fate worse than death :D 
// #RETURN : status
// 	0 on error
// 	1 on success
int llistRemove(struct llist* list, int index); 

// free all nodes in linked list
// @list: list to free
void llistFree(struct llist* list);


#endif
