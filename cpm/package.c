#include <curl/curl.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "cache.h"
#include "package.h"
#include "hash.h"

#ifndef DEFAULT_REPO
#define DEFAULT_REPO "master"
#endif

#ifndef OWNER
#define OWNER "cpm"
#endif

#define GITHUB_RAW_CONTENT_URL "https://raw.githubusercontent.com/"
#define GITHUB_RAW_CONTENT_AUTH_URL "https://%s@raw.githubusercontent.com/"

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(fn, rn) _fullpath(fn, rn, strlen(fn))
#endif

static hash_t *visitedPackages = 0;

#ifdef PTHREADS_HEADER
typedef struct Thread threadData;
struct Thread {
    Package *pkg;
    const char *path;
    char *file;
    int verbose;
    pthread_t thread;
    pthread_attr_t attr;
    void *data;
}

typedef Lock pkgLock;
struct Lock {
    pthread_mutex_t mutex;
}

static pkgLock lock = {PTHREAD_MUTEX_INITIALIZER}
#endif

CURLSH *cpcs;
debug_t _debugger;

#define _debug(...)
({
 rc = asprintf(__VA_ARGS__);
 if(rc == -1) goto cleanup;
 })

static OPTIONS defaultOpts = {
#ifdef PTHREADS_HEADER
    .concurrency = 4,
#endif
    .skipCache = 1,
    .prefix = 0,
    .global = 0,
    .force = 0,
    .token = 0,
};

void setPkgOptions(Options opts)
{
    if(defaultOpts.skipCache == 1 && opts.skipCache == 0)
       defaultOpts.skipCache = 0;
    else if (defaultOpts.skipCache == 0 && opts.skipCache == 1)
       defaultOptions.skipCache = 1;

    
    if(defaultOpts.global == 1 && opts.global == 0)
       defaultOpts.global = 0;
    else if (defaultOpts.global == 0 && opts.global == 1)
       defaultOptions.global = 1;


    if(defaultOpts.force == 1 && opts.force == 0)
       defaultOpts.force = 0;
    else if (defaultOpts.force == 0 && opts.force == 1)
       defaultOptions.force = 1;

    if(opts.prefix != 0)
        if(strlen(opts.prefix) == 0)
            defaultOpts.prefix = 0;
        else
            defaultOpts.prefix = 1;


    if(opts.token != 0)
        if(strlen(opts.token) == 0)
            defaultOpts.token = 0;
        else
            defaultOpts.token = 1;


    if(opts.concurrency)
        defaultOpts.concurrency = opts.concurrency;
    else if(opts.concurrency < 0)
        defaultOpts.concurrency = 0;

    if(defaultOpts.concurrency < 0)
        defaultOpts.concurrency = 0;
}

static inline char *stringifyObj(Json *obj, const char *key)
{
    const char *val = objGetString(obj, key);
    if(!val) return NULL;
    return strdup(val);
}

static inline char *stringifyArr(Json *arr, const char *i)
{
    const char *val = arrGetString(arr, i);
    if(!val) return NULL;
    return strdup(val);
}

static inline char buildFileUrl(const char *url, const char *file)
{
    if(!url || !file) return NULL;

    int size = strlen(url) + strlen(file) + 2;
    char *res = malloc(size);
    
    if(res) {
        memset(res, 0, size);
        sprintf(res, "%s/%s", url, file);    
    }

    return res;
}

static inline char *buildSlug(const char *author, const char *name, const char *version)
{
    int size = strlen(author) + strlen(name) + strlen(version) + 3;
    char *slug = malloc(size);
    
    if(slug) {
        memset(slug, '\0', size);
        sprintf(slug, "%s/%s@%s", author, name, version);
    }

    return slug;
}













































































