#include <stdio.h>
#include <stdlib.h>
#include "list.h"

// Concrete implementation in main, module remains generic
void *square(void *n) {
    int *val = malloc(sizeof(int));
    *val = (*(int*)n) * (*(int*)n);
    return val;
}

int main() {
    int a = 1, b = 2, c = 3;
    Node *list = cons(&a, cons(&b, cons(&c, NULL)));
    
    Node *squared = map(list, square);
    
    Node *curr = squared;
    while (curr) {
        printf("%d ", *(int*)curr->value);
        curr = curr->next;
    }
    printf("\n");
    
    // Cleanup - simplified for example
    free_list(list);
    free_list(squared);
    
    return 0;
}
