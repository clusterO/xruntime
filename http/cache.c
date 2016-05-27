#include "cache.h"

/**
 * Allocate a new CacheEntry struct and initialize its members.
 *
 * @param path          The path of the cache entry.
 * @param contentType   The content type of the cache entry.
 * @param content       The content of the cache entry.
 * @param contentLength The length of the content.
 * @return              A pointer to the newly allocated CacheEntry struct.
 */
struct CacheEntry *allocateEntry(char *path, char *contentType, void *content, int contentLength)
{
    struct CacheEntry *entry = malloc(sizeof(struct CacheEntry));

    entry->path = strdup(path);
    entry->type = strdup(contentType);
    entry->length = contentLength;
    entry->content = malloc(contentLength);
    memcpy(entry->content, content, contentLength);
    entry->prev = NULL;
    entry->next = NULL;

    return entry;
}

/**
 * Free the memory occupied by a CacheEntry struct.
 *
 * @param entry   The CacheEntry struct to free.
 */
void freeEntry(struct CacheEntry *entry)
{
    free(entry->path);
    free(entry->type);
    free(entry->content);
    free(entry);
}

/**
 * Create a new Cache struct with the specified maximum size and hash size.
 *
 * @param maxSize   The maximum size of the cache.
 * @param hashSize  The size of the hash table used for indexing cache entries.
 * @return          A pointer to the created Cache struct.
 */
struct Cache *createCache(int maxSize, int hashSize)
{
    struct Cache *cache = malloc(sizeof(struct Cache));

    cache->index = hcreate(hashSize, NULL); // NULL to use default hash function
    cache->head = NULL;
    cache->tail = NULL;
    cache->maxSize = maxSize;
    cache->size = 0; // Initialize the size to 0

    printf("cache[%d,%d]created\n", cache->maxSize, cache->index->size);
    return cache;
}

/**
 * Add a new entry to the cache.
 *
 * @param cache          The cache where the entry will be added.
 * @param path           The path of the entry.
 * @param contentType    The content type of the entry.
 * @param content        A pointer to the content of the entry.
 * @param contentLength  The length of the content in bytes.
 */
void cput(struct Cache *cache, char *path, char *contentType, void *content, int contentLength)
{
    struct CacheEntry *entry = allocateEntry(path, contentType, content, contentLength);

    dllInsertHead(cache, entry);

    hput(cache->index, path, entry);
    cache->size++;

    if (cache->size > cache->maxSize)
    {
        struct CacheEntry *tempEntry = dllRemoveTail(cache);
        hdelete(cache->index, tempEntry->path);
        freeEntry(tempEntry);
    }

    freeEntry(entry);
}

/**
 * Get an entry from the cache.
 *
 * @param cache  The cache to retrieve the entry from.
 * @param path   The path of the entry to retrieve.
 *
 * @return       The cache entry if found, or NULL if not found.
 */
struct CacheEntry *cget(struct Cache *cache, char *path)
{
    struct CacheEntry *entry = hget(cache->index, path);

    if (entry == NULL)
        return NULL;
    else
    {
        dllMoveToHead(cache, entry);
        return entry;
    }
}

/**
 * Insert an entry at the head of the cache.
 *
 * @param cache  The cache to insert the entry into.
 * @param ce     The entry to be inserted.
 */
void dllInsertHead(struct Cache *cache, struct CacheEntry *ce)
{
    if (cache->head == NULL)
    {
        // If the cache is empty, set the head and tail to the new entry
        cache->head = cache->tail = ce;
        ce->prev = ce->next = NULL;
    }
    else
    {
        // Otherwise, update the references to insert the entry at the head
        cache->head->prev = ce;
        ce->next = cache->head;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Move an entry to the head of the cache.
 *
 * @param cache  The cache containing the entry.
 * @param ce     The entry to be moved.
 */
void dllMoveToHead(struct Cache *cache, struct CacheEntry *ce)
{
    if (ce != cache->head)
    {
        if (ce == cache->tail)
        {
            // If the entry is the tail, update the tail reference and its previous reference
            cache->tail = ce->prev;
            cache->tail->next = NULL;
        }
        else
        {
            // Otherwise, update the references of the neighboring entries
            ce->prev->next = ce->next;
            ce->next->prev = ce->prev;
        }

        // Update the references to insert the entry at the head
        ce->next = cache->head;
        cache->head->prev = ce;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Remove the tail entry from the cache and return it.
 *
 * @param cache  The cache from which to remove the tail entry.
 * @return       The removed tail entry.
 */
struct CacheEntry *dllRemoveTail(struct Cache *cache)
{
    struct CacheEntry *oldTail = cache->tail;
    cache->tail = oldTail->prev;
    cache->tail->next = NULL;
    cache->size--;
    return oldTail;
}

/**
 * Free the cache and its associated memory.
 *
 * @param cache  The cache to be freed.
 */
void freeCache(struct Cache *cache)
{
    struct CacheEntry *entry = cache->head;
    hdestroy(cache->index);

    while (entry != NULL)
    {
        struct CacheEntry *nextEntry = entry->next;
        freeEntry(entry);
        entry = nextEntry;
    }

    free(cache);
}