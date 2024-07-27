#ifndef _WEB_CACHE_HEADER_
#define _WEB_CACHE_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

struct CacheEntry
{
    char *path;
    char *type;
    int length;
    void *content;

    struct CacheEntry *prev, *next;
};

struct Cache
{
    struct Hashtable *index;
    struct CacheEntry *head, *tail;
    int maxSize;
    int size;
};

extern struct CacheEntry *allocateEntry(char *path, char *contentType, void *content, int contentLength);
extern void freeEntry(struct CacheEntry *entry);
extern struct Cache *createCache(int maxSize, int hashSize);
extern void freeCache(struct Cache *cache);
extern void cput(struct Cache *cache, char *path, char *contentType, void *content, int contentLength);
extern struct CacheEntry *cget(struct Cache *cache, char *path);

struct CacheEntry *dllRemoveTail(struct Cache *cache);
void dllMoveToHead(struct Cache *cache, struct CacheEntry *ce);
void dllInsertHead(struct Cache *cache, struct CacheEntry *ce);

#endif