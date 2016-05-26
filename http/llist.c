#include <stdlib.h>

#include "llist.h"

struct llist *llcreate(void)
{
    return calloc(1, sizeof(struct llist));
}

void lldestroy(struct llist *llist)
{
    struct Node *n = llist->head, *next;
    while (n != NULL)
    {
        next = n->next;
        free(n);
        n = next;
    }

    free(llist);
}

void *llinsert(struct llist *llist, void *data)
{
    struct Node *n = calloc(1, sizeof *n);
    if (n == NULL)
        return NULL;

    n->data = data;
    n->next = llist->head;
    llist->head = n;
    llist->count++;

    return data;
}

void *llappend(struct llist *llist, void *data)
{
    struct Node *tail = llist->head;
    if (tail == NULL)
        return NULL;
    struct Node *n = calloc(1, sizeof *n);
    if (n == NULL)
        return NULL;

    while (tail->next != NULL)
        tail = tail->next;

    n->data = data;
    tail->next = n;
    llist->count++;

    return data;
}

void *llhead(struct llist *llist)
{
    if (llist->head == NULL)
        return NULL;
    return llist->head->data;
}

void *lltail(struct llist *llist)
{
    struct Node *n = llist->head;
    if (n == NULL)
        return NULL;

    while (n->next != NULL)
        n = n->next;

    return n->data;
}

void *llfind(struct llist *llist, void *data, int (*cmpfn)(void *, void *))
{
    struct Node *n = llist->head;
    if (n == NULL)
        return NULL;

    while (n != NULL)
    {
        if (cmpfn(data, n->data) == 0)
            break;

        n = n->next;
    }

    if (n == NULL)
        return NULL;
    return n->data;
}

void *lldelete(struct llist *llist, void *data, int (*cmpfn)(void *, void *))
{
    struct Node *n = llist->head, *prev = NULL;
    while (n != NULL)
    {
        if (cmpfn(data, n->data) == 0)
        {
            void *data = n->data;
            if (prev == NULL)
            {
                llist->head = n->next;
                free(n);
            }
            else
            {
                prev->next = n->next;
                free(n);
            }
            llist->count--;
            return data;
        }
        prev = n;
        n = n->next;
    }
    return NULL;
}

void llforEach(struct llist *llist, void (*f)(void *, void *), void *arg)
{
    struct Node *p = llist->head, *next;
    while (p != NULL)
    {
        next = p->next;
        f(p->data, arg);
        p = next;
    }
}

void **llgetArray(struct llist *llist)
{
    if (llist->head == NULL)
        return NULL;
    void **a = malloc(sizeof *a * llist->count + 1);
    struct Node *n;
    int i;

    for (i = 0, n = llist->head; n != NULL; i++, n = n->next)
        a[i] = n->data;

    a[i] = NULL;
    return a;
}

void llfreeArray(void **a)
{
    free(a);
}