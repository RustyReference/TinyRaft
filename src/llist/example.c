// an example usage of llist.h
#include "../include.h"


// print a linked list of int types with newline at end
// @list : list to print
void printList(struct llist list);

int main() {
	// make a new llist
	struct llist list = llistCreat();

	// this is a not-so-surprise tool that will help me later
	int status, arr[5] = {1, 2, 3, 4, 5};

	// add something to index 0;
	printf("adding 1\n");
	assert(llistInsert(&list, 0, &arr[0]));
	printList(list);

	// add something to next index
	printf("adding 2\n");
	assert(llistAdd(&list, &arr[1]));
	printList(list);

	// remove first value
	printf("removing 1\n");
	assert(llistRemove(&list, 0));
	printList(list);

	// add all elements from last to first and end program
	printf("adding all backwards\n");
	for(int i = 4; i >= 0; i--) {
		assert(llistAdd(&list, &arr[i]));
		printList(list);
	}

	// free and exit
	llistFree(&list);
	return 0;
}

// print a linked list of int types
// @list : list to print
void printList(struct llist list) {
	struct node* head = list.first;
	while(head) {
		int* info = (int*)head->data;
		printf("%d ", *info);
		head = head->next;
	}
	putchar('\n');
}
