#ifndef _HASHTABLE_HEADER_
#define _HASHTABLE_HEADER_

struct Hashtable {
    int size; 
    int numEntries; 
    float load; 
    struct llist **bucket;
    int (*hashf)(void *data, int dataSize, int bucketCount);
};

extern struct Hashtable *createHTable(int size, int (*hashf)(void *, int, int));
extern void destroyHTable(struct Hashtable *ht);
extern void *put(struct Hashtable *ht, char *key, void *data);
extern void *putBin(struct Hashtable *ht, void *key, int key_size, void *data);
extern void *get(struct Hashtable *ht, char *key);
extern void *getBin(struct Hashtable *ht, void *key, int key_size);
extern void *deleteHTable(struct Hashtable *ht, char *key);
extern void *deleteHTableBin(struct Hashtable *ht, void *key, int key_size);
extern void forEach(struct Hashtable *ht, void (*f)(void *, void *), void *arg);

#endif