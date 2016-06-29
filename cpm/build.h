#ifndef _BUILD_HEADER_
#define _BUILD_HEADER_

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#ifdef _WIN32
#include <direct.h>
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

// to be moved
#ifdef PTHREADS_HEADER
static void setConcurency(command_t *self)
{
    if (self->arg)
    {
        options.concurrency = atol(self->arg);
        debug(&debugger, "set concurrency: %lu", options.concurrency);
    }
}
#endif

// to be moved
#ifdef PTHREADS_HEADER
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Thread
{
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>

#include "common/cache.h"
#include "common/package.h"
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

typedef struct
{
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
} BuildOptions;

static void setCache(command_t *self);
static void setDev(command_t *self);
static void setForce(command_t *self);
static void setGlobal(command_t *self);
static void setClean(command_t *self);
static void setTest(command_t *self);
static void setPrefix(command_t *self);
static void setDir(command_t *self);
static void unsetVerbose(command_t *self);

int buildPackage(const char *dir);

#endif