#ifndef _LLIST_HEADER_
#define _LLIST_HEADER_

struct llist {
	struct node *head;
	int count;
};

extern struct llist *createLList(void);
extern void destroy(struct llist *llist);
extern void *insert(struct llist *llist, void *data);
extern void *append(struct llist *llist, void *data);
extern void *head(struct llist *llist);
extern void *tail(struct llist *llist);
extern void *find(struct llist *llist, void *data, int (*cmpfn)(void *, void *));
extern void *deleteL(struct llist *llist, void *data, int (*cmpfn)(void *, void *));
extern int count(struct llist *llist);
extern void forEach(struct llist *llist, void (*f)(void *, void *), void *arg);
extern void **getArray(struct llist *llist);
extern void freeArray(void **a);

#endif