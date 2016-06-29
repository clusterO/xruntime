#ifndef _CONFIGURE_HEADER_
#define _CONFIGURE_HEADER_

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#endif

#ifdef PTHREADS_HEADER
#define MAX_THREADS 4
#endif

#define PACKAGE_CACHE_TIME 2592000

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

// move to source file, or to upper file for communs
#ifdef PTHREADS_HEADER
static void setConcurrency(command_t *self)
{
    if (self->arg)
    {
        options.concurrency = atol(self->arg);
        debug(&debugger, "set concurrenc: %lu", options.concurrency);
    }
}
#endif

// move to source file, or to upper file for communs
#ifdef PTHREADS_HEADER
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Thread
{
    const char *dir;
};

void *configurePackageThread(void *arg)
{
    Thread *wrap = arg;
    const char *dir = wrap->dir;
    configurePackage(dir);
    return 0;
}
#endif

#include <curl/curl.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

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
#include "common/cache.h"
#include "common/package.h"

typedef struct
{
    const char *dir;
    char *prefix;
    int force;
    int verbose;
    int dev;
    int skipCache;
    int flags;
    int global;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
} ConfigOptions;

static void setCache(command_t *self);
static void setDev(command_t *self);
static void setForce(command_t *self);
static void setGlobal(command_t *self);
static void setFlags(command_t *self);
static void setPrefix(command_t *self);
static void setDir(command_t *self);
static void unsetVerbose(command_t *self);

int configurePackage(const char *dir);

#endif