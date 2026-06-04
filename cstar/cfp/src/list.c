#include <stdlib.h>
#include "list.h"

Node *cons(void *value, Node *next) {
    Node *node = malloc(sizeof(Node));
    node->value = value;
    node->next = next;
    return node;
}

Node *map(Node *list, void *(*f)(void *)) {
    if (!list) return NULL;
    return cons(f(list->value), map(list->next, f));
}

void free_list(Node *list) {
    while (list) {
        Node *temp = list;
        list = list->next;
        free(temp);
    }
}
