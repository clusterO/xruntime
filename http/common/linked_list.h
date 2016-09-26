#ifndef _LINKED_LIST_HEADER_
#define _LINKED_LIST_HEADER_

#include <stdlib.h>

struct llist
{
	struct Node *head;
	int count;
};

struct Node
{
	void *data;
	struct Node *next;
};

extern struct llist *llcreate(void);
extern void lldestroy(struct llist *llist);
extern void *llinsert(struct llist *llist, void *data);
extern void *llappend(struct llist *llist, void *data);
extern void *llhead(struct llist *llist);
extern void *lltail(struct llist *llist);
extern void *llfind(struct llist *llist, void *data, int (*cmpfn)(void *, void *));
extern void *lldelete(struct llist *llist, void *data, int (*cmpfn)(void *, void *));
extern int llcount(struct llist *llist);
extern void llforEach(struct llist *llist, void (*f)(void *, void *), void *arg);
extern void **llgetArray(struct llist *llist);
extern void llfreeArray(void **a);

#endif
