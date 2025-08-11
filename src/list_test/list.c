#include "../server/thread/thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

struct Node {
    char *str;
    SLIST_ENTRY(Node)
    entries;
};

SLIST_HEAD(List, Node);

void push(struct List *list, char *val) {
    struct Node *n1 = malloc(sizeof(struct Node));
    n1->str = "Apple pen";
    SLIST_INSERT_HEAD(list, n1, entries);
}

void pop(struct List *list) {
    struct Node *head = SLIST_FIRST(list);
    SLIST_REMOVE_HEAD(list, entries);
    free(head);
}

void removeAfter(struct List *list, char *str) {
    struct Node *curr;
    struct Node *prev = NULL;

    SLIST_FOREACH(curr, list, entries) {
        if (!(strncmp(curr->str, str, 255) == 0)) {
            continue;
        }

        if (prev != NULL) {
            SLIST_REMOVE_AFTER(prev, entries);
        } else {
            SLIST_REMOVE_HEAD(list, entries);
        }

        prev = curr;
    }
}

void deleteList(struct List *list) {
    while (!SLIST_EMPTY(list)) {
        struct Node *n = SLIST_FIRST(list);
        SLIST_REMOVE_HEAD(list, entries);
        free(n);
    }
}

#define BUF_LIM 256
void serializeEntry(struct LogEntry entryInfo, char buff[BUF_LIM]) {
    snprintf(buff, BUF_LIM, "%d %d %s %d",
             entryInfo.index,
             entryInfo.term,
             entryInfo.command,
             entryInfo.cmdlen);
}

int main() {
    struct LogEntry entry = {
        1, 1, "test", 4
    };

    char s[BUF_LIM];
    serializeEntry(entry, s);
    printf("Serialized: %s", s);

    return 0;
}