#ifndef LIST_H
#define LIST_H

typedef struct Node {
    void *value; // Polymorphic value
    struct Node *next;
} Node;

Node *cons(void *value, Node *next);
Node *map(Node *list, void *(*f)(void *));
void free_list(Node *list);

#endif
