#ifndef _LLIST_HEADER_
#define _LLIST_HEADER_

struct llist {
	struct node *head;
	int count;
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
