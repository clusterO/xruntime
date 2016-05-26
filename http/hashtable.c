#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "llist.h"
#include "hashtable.h"

#define DEFAULT_SIZE 128
#define DEFAULT_GROW_FACTOR 2

void addEntryCount(struct Hashtable *ht, int d)
{
    ht->numEntries += d;
    ht->load = (float)ht->numEntries / ht->size;
}

// Modulo hash
int defaultHashF(void *data, int dataSize, int bucketCount)
{
    const unsigned char *p = (const unsigned char *)data;
    unsigned int h = 0;

    for (int i = 0; i < dataSize; i++)
    {
        h = (h * 31 + p[i]) % bucketCount;
    }

    return h;
}

struct Hashtable *hcreate(int size, int (*hashf)(void *, int, int))
{
    if (size < 1)
        size = DEFAULT_SIZE;
    if (hashf == NULL)
        hashf = defaultHashF;

    struct Hashtable *ht = malloc(sizeof(struct Hashtable));
    if (ht == NULL)
        return NULL;

    ht->size = size;
    ht->numEntries = 0;
    ht->load = 0;
    ht->bucket = malloc(size * sizeof(struct llist *));
    ht->hashf = hashf;

    if (ht->bucket == NULL)
    {
        free(ht);
        return NULL;
    }

    for (int i = 0; i < size; i++)
    {
        ht->bucket[i] = llcreate();
        /*
        if (ht->bucket[i] == NULL)
        {
            // Cleanup allocated buckets
            for (int j = 0; j < i; j++)
                llfree(ht->bucket[j]);
            free(ht->bucket);
            free(ht);
            return NULL;
        }*/
    }

    return ht;
}

void freeHtent(void *htent, void *arg)
{
    free(htent);
    (void)arg; // Unused parameter
}

void hdestroy(struct Hashtable *ht)
{
    for (int i = 0; i < ht->size; i++)
    {
        struct llist *llist = ht->bucket[i];
        llforEach(llist, freeHtent, NULL);
        lldestroy(llist);
    }

    free(ht);
}

void *hput(struct Hashtable *ht, char *key, void *data)
{
    return hputBin(ht, key, strlen(key), data);
}

void *hputBin(struct Hashtable *ht, void *key, int keySize, void *data)
{
    int index = ht->hashf(key, keySize, ht->size);
    struct llist *llist = ht->bucket[index];

    struct Htent *ent = malloc(sizeof(struct Htent));
    if (ent == NULL)
        return NULL;

    ent->key = malloc(keySize);
    if (ent->key == NULL)
    {
        free(ent);
        return NULL;
    }

    memcpy(ent->key, key, keySize);
    ent->keySize = keySize;
    ent->hashedKey = index;
    ent->data = data;

    if (llappend(llist, ent) == NULL)
    {
        free(ent->key);
        free(ent);
        return NULL;
    }

    addEntryCount(ht, 1);
    return data;
}

int htcmp(void *a, void *b)
{
    struct Htent *entA = (struct Htent *)a;
    struct Htent *entB = (struct Htent *)b;

    int sizeDiff = entB->keySize - entA->keySize;
    if (sizeDiff != 0)
        return sizeDiff;

    return memcmp(entA->key, entB->key, entA->keySize);
}

void *hget(struct Hashtable *ht, char *key)
{
    return hgetBin(ht, key, strlen(key));
}

void *hgetBin(struct Hashtable *ht, void *key, int keySize)
{
    int index = ht->hashf(key, keySize, ht->size);
    struct llist *llist = ht->bucket[index];

    struct Htent cmpent;
    cmpent.key = key;
    cmpent.keySize = keySize;

    struct Htent *n = llfind(llist, &cmpent, htcmp);
    if (n != NULL)
        return n->data;

    return NULL;
}

void *hdelete(struct Hashtable *ht, char *key)
{
    return hdeleteBin(ht, key, strlen(key));
}

void *hdeleteBin(struct Hashtable *ht, void *key, int keySize)
{
    int index = ht->hashf(key, keySize, ht->size);
    struct llist *llist = ht->bucket[index];

    struct Htent cmpent;
    cmpent.key = key;
    cmpent.keySize = keySize;

    struct Htent *ent = lldelete(llist, &cmpent, htcmp);
    if (ent == NULL)
        return NULL;

    void *data = ent->data;
    free(ent);

    addEntryCount(ht, -1);
    return data;
}

void forEachPayload(void *vent, void *vpayload)
{
    struct Htent *ent = vent;
    struct Payload *payload = vpayload;
    payload->f(ent->data, payload->arg);
}

void hforEach(struct Hashtable *ht, void (*f)(void *, void *), void *arg)
{
    struct Payload payload;
    payload.f = f;
    payload.arg = arg;

    for (int i = 0; i < ht->size; i++)
    {
        struct llist *llist = ht->bucket[i];
        llforEach(llist, forEachPayload, &payload);
    }
}
