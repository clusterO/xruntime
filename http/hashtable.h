#ifndef _HASHTABLE_HEADER_
#define _HASHTABLE_HEADER_

struct Hashtable {
    int size; 
    int numEntries; 
    float load; 
    struct llist **bucket;
    int (*hashf)(void *data, int dataSize, int bucketCount);
};

extern struct Hashtable *hcreate(int size, int (*hashf)(void *, int, int));
extern void hdestroy(struct Hashtable *ht);
extern void *hput(struct Hashtable *ht, char *key, void *data);
extern void *hputBin(struct Hashtable *ht, void *key, int keySize, void *data);
extern void *hget(struct Hashtable *ht, char *key);
extern void *hgetBin(struct Hashtable *ht, void *key, int keySize);
extern void *hdelete(struct Hashtable *ht, char *key);
extern void *hdeleteBin(struct Hashtable *ht, void *key, int keySize);
extern void hforEach(struct Hashtable *ht, void (*f)(void *, void *), void *arg);

#endif
