#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "cache.h"

struct CacheEntry *allocateEntry(char *path, char *contentType, void *content, int contentLength) {}
void freeEntry(struct CacheEntry *entry) {}
struct Cache *createCache(int maxSize, int hashSize) {}
void cput(struct Cache *cache, char *path, char *contentType, void *content, int contentLength) {}
struct CacheEntry *cget(struct Cache *cache, char *path) {}

void dllInsertHead(struct Cache *cache, struct CacheEntry *ce)
{
    if(cache->head == NULL) {
        cache->head = cache->tail = ce;
        ce->prev = ce->next = NULL;
    } else {
        cache->head->prev = ce;
        ce->next = cache->head;
        ce->prev = NULL;
        cache->head = ce;
    }
}

void dllMoveToHead(struct Cache *cache, struct CacheEntry *ce)
{
    if(ce != cache->head) {
        if(ce == cache->tail) {
            cache->tail = ce->prev;
            cache->tail->prev = NULL;
        } else {
            ce->prev->next = ce->next;
            ce->next->prev = ce->prev;
        }

        ce->next = cache->head;
        cache->head->prev = ce;
        ce->prev = NULL;
        cache->head = ce;
    }
}

struct CacheEntry *dllRemoveTail(struct Cache *cache)
{
    struct CacheEntry *oldtail = cache->tail;
    cache->tail = oldtail->prev;
    cache->tail->next = NULL;
    cache->size--;
    return oldtail;
}

void freeCache(struct Cache *cache)
{
    struct  CacheEntry *entry = cache->head;
    hdestroy(cache->index);
    
    while(entry != NULL) {
        struct CacheEntry *nextEntry = entry->next;
        freeEntry(entry);
        entry = nextEntry;
    }
    free(cache);
}





























