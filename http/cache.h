#ifndef _WEBCACHE_HEADER_
#define _WEBCACHE_HEADER_

struct CacheEntry {
    char *path;
    char *type;
    int length;
    void *content;

    struct CacheEntry *prev, *next;
};


struct Cache {
    struct Hashtable *index;
    struct CacheEntry *head, *tail;
    int maxSize;
    int size;
};

extern struct CacheEntry *allocateEntry(char *path, char *contentType, void *content, int contentLength);
extern void freeEntry(struct CacheEntry *entry);
extern struct Cache *createCache(int max_size, int hashsize);
extern void freeCache(struct Cache *cache);
extern void cput(struct Cache *cache, char *path, char *contentType, void *content, int contentLength);
extern struct CacheEntry *cget(struct Cache *cache, char *path);

#endif
