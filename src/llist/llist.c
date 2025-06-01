#include "../include.h"

// initialize a linked list. 
// #RETURN : new linked list.
struct llist llistCreat() {
	struct llist list;
	list.last = list.first = NULL;
	return list;
}


// add data to a linked list
// @list : list to add to
// @data : data to add
// #RETURN : status
// 	0 on error
// 	1 on success
int llistAdd(struct llist* list, void* data) {
	// sanitize
	if(!list) { return 0; }
	if(!data) { return 0; }

	// get the last node
	struct node* tail = list->last;
	if(!tail) {
		tail = (struct node*)malloc(sizeof(struct node));
	} else {
		tail->next = (struct node*)malloc(sizeof(struct node));
		tail = tail->next;
	}
	if(!list->first) { list->first = list->last; }

	// insert the data and return
	tail->next = NULL; 
	tail->data = data;
	list->last = tail;
	return 1;
}


// insert data to a linked list's index
// @list  : list to add to
// @index : exact index you want data to end up in
// @data  : info to add
// #RETURN : status
// 	0 on error
// 	1 on success
int llistInsert(struct llist* list, int index, void* data) {
	// sanitize 
	if(!list) { return 0; }
	if(!data) { return 0; }
	if(index < 0) { return 0; }

	// get the index node
	struct node* prevNode = NULL;
	struct node* nextNode = list->first;
	for(int i = 0; i < index; i++) {
		if(!nextNode) { return 0; }
		prevNode = nextNode;
		nextNode = nextNode->next;
	}

	// insert the new node
	struct node* newNode = (struct node*)malloc(sizeof(struct node));
	
	// handle previous node
	if(prevNode) { prevNode->next = newNode; }
	else { list->first = newNode; }

	// handle next node
	if(!nextNode) { list->last = newNode; }
	newNode->next = nextNode;

	// insert data and return
	newNode->data = data;
	return 1;
}


// remove data from a linked list's index
// @list  : list to remove from
// @index : exact index you want gone. Memory will be freed. Existence will be forgotten. Fate worse than death :D 
// #RETURN : status
// 	0 on error
// 	1 on success
int llistRemove(struct llist* list, int index) {
	// sanitize
	if(!list) { return 0; }
	if(index < 0) { return 0; }

	// get to the index
	struct node* prevNode = NULL;
	struct node* headNode = list->first;
	for(int i = 0; i < index; i++) {
		if(!headNode) { return 0; }
		prevNode = headNode;
		headNode = headNode->next;
	}

	// handle previous and next
	if(prevNode) { prevNode->next = headNode->next; }
	else { list->first = headNode->next; }
	if(!headNode->next) { list->last = headNode->next; }

	// delete index and return
	free(headNode);
	return 1;
}


// free all nodes in linked list
// @list: list to free
void llistFree(struct llist* list) {
	// loop through hwile freeing 
	struct node *temp, *head = list->first;
	while(head) {
		temp = head->next;
		free(head);
		head = temp;
	}
}

// get data from a linked list
// @list  : list to get from
// @index : exact index you want to get
// #RETURN : pointer
// 	NULL on error
// 	a pointer to the value on success
void* llistGet(struct llist* list, int index) {
	//sanitize 
	if(!list) { return NULL; }
	if(index < 0) { return NULL; }

	// get to the index
	struct node* head = list->first;
	for(int i = 0; i < index; i++) {
		if(!head) { return NULL; }
		head = head->next;
	}

	// return the pointer
	return head->data;
}
