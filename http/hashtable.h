#ifndef _HASHTABLE_HEADER_
#define _HASHTABLE_HEADER_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "llist.h"

#define DEFAULT_SIZE 128
#define DEFAULT_GROW_FACTOR 2

struct Hashtable
{
    int size;
    int numEntries;
    float load;
    struct llist **bucket;
    int (*hashf)(void *data, int dataSize, int bucketCount);
};

struct Htent
{
    void *key;
    int keySize;
    int hashedKey;
    void *data;
};

// linked list cleaner
struct Payload
{
    void *arg;
    void (*f)(void *, void *);
};

extern struct Hashtable *hcreate(int size, int (*hashf)(void *, int, int));
extern void hdestroy(struct Hashtable *ht);
extern void *hput(struct Hashtable *ht, char *key, void *data);
extern void *hget(struct Hashtable *ht, char *key);
extern void *hdelete(struct Hashtable *ht, char *key);
extern void hforEach(struct Hashtable *ht, void (*f)(void *, void *), void *arg);

#endif
