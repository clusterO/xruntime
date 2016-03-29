#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <direct.h>
#include "cache.h"
#include "package.h"
#include "libs/asprintf.h"
#include "libs/commander.h"
#include "libs/debug.h"
#include "libs/fs.h"
#include "libs/hash.h"
#include "libs/list.h"
#include "libs/logger.h"
#include "libs/path-join.h"
#include "libs/str-flatten.h"
#include "libs/trim.h"

#ifdef _WIN32
#define getcwd _getcwd
#endif

#define PACKAGE_CACHE_TIME 2592000

#ifdef PTHREADS_HEADER
#define MAX_THREADS 4
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

struct options {
    const char *dir;
    char *prefix;
    int force;
    int verbose;
    int dev;
    int skipCache;
    int global;
    char *clean;
    char *test;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
};

Options pkgOpts = {0};
Package rootPkg = NULL;
debug_t debugger = {0};
hash_t *built = 0;
char **argV = 0;
int argC = 0;
int offset = 0;

options opts = {
    .skipCache = 0,
    .verbose = 1,
    .force = 0,
    .dev = 0,
#ifdef PTHREADS_HEADER
    .concurrency = MAX_THREADS,
#endif
#ifdef _WIN32
    .dir = ".\\deps",
#else
    .dir = "./deps",
#endif
};

int buildPackage(const char *dir);

#ifdef PTHREADS_HEADER
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Thread {
    const char *dir;
};

void *buildPackageThread(void *arg) 
{
    Thread *wrap = arg;
    const char *dir = wrap->dir;
    buildPackage(dir);
    return 0;
}
#endif

int buildPackage(const char *dir)
{
    const char *file = "manifest.json"; 
    Package *pkg = NULL;
    char *json = NULL;
    int ok = 0;
    int rc = 0;
    long pathMax;
#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif defined(_PC_PATH_MAX)
    pathMax = pathconf(dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

    char *path = path_join(dir, file);

    if(path == 0) return -ENOMEM;

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&mutex);
#endif

    if(!rootPkg) {
        char *json = fs_read(file);
        if(json)
            rootPkg = newPkg(json, opts.verbose);

        if(rootPkg && rootPkg->prefix) {
            char prefix[pathMax];
            memset(prefix, 0, pathMax);
            realpath(rootPkg->prefix, prefix);
            unsigned long int size = strlen(prefix) + 1;
            free(rootPkg->prefix);
            rootPkg->prefix = malloc(size);
            memset((void *)rootPkg->prefix, 0, size);
            memcpy((void *)rootPkg->prefix, prefix, size);
        }
    }

    if(hash_has(built, path)) {
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&mutex);
#endif
        goto clean;
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&mutex);
#endif

    if(fs_exists(path)) {
        debug(&debugger, "reading %s", path);
        json = fs_read(path);
    }

    if(json != 0) {
#ifdef DEBUG
        pkg = newPkg(json, 1);
#else
        pkg = newPkg(json, 0);
#endif
    } else {
#ifdef DEBUG
        pkg = newPkgSlug(dir, 1);
#else
        pkg = newPkgSlug(dir, 0);
#endif
    }
}
































