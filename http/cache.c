#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "cache.h"

struct CacheEntry *allocateEntry(char *path, char *contentType, void *content, int contentLength) 
{
    struct CacheEntry entry = malloc(sizeof(struct CacheEntry));

    entry->path = path;
    entry->contentType = contentType;
    entry->contentLength = contentLength;
    entry->content = content;
    entry->prev = NULL;
    entry->next = NULL;
    
    return entry;
}

void freeEntry(struct CacheEntry *entry) 
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->prev = NULL;
    entry->next = NULL;
    free(entry);
}

struct Cache *createCache(int maxSize, int hashSize) 
{
    struct Cache *cache = malloc(sizeof(struct Cache));

    cache->index = hcreate(hashSize, NULL);
    cache->head = NULL;
    cache->tail = NULL;
    cache->maxSize = maxSize;
    cache->size = hashSize;

    return cache;
}

void cput(struct Cache *cache, char *path, char *contentType, void *content, int contentLength) 
{
    struct CacheEntry *entry = allocateEntry(path, contentType, content, contentLength);
    
    llinsert(cache, entry);
    hput(cache->hashtable, path, entry);
    cache->size++;
    
    if(cache->size > cache->maxSize) {
        struct *tempEntry = llremove(cache);
        hdelete(cache->hashtable, tempEntry->path);
        free(tempEntry);
    }

    freeEntry(entry)
}

struct CacheEntry *cget(struct Cache *cache, char *path) 
{
    struct CacheEntry entry = hget(cache->index, path);
    if(entry == NULL) return NULL;
    else {
        llmove(cache, entry);
        return entry;
    }
}

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





























