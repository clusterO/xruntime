#ifndef _UPGRADE_HEADER_
#define _UPGRADE_HEADER_

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#ifdef PTHREADS_HEADER
#define MAX_THREADS 16
#endif

#define PACKAGE_CACHE_TIME 2592000

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

// Enhance - to be moved to a larger scope
#ifdef PTHREADS_HEADER
static void setConcurrency(command_t *self)
{
    if (self->arg)
    {
        options.concurrency = atol(self->arg);
        debug(&debugger, "set concurrency: %lu", options.concurrency);
    }
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <curl/curl.h>

#include "libs/asprintf.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/logger.h"
#include "libs/debug.h"
#include "libs/commander.h"
#include "common/cache.h"
#include "common/package.h"
#include "install.h" // used functions should move to common

typedef struct
{
    char *prefix;
    char *token;
    char *slug;
    char *tag;
    char *dir;
    int verbose;
    int force;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
} UpgradeOptions;

static void setSlug(command_t *self);
static void setTag(command_t *self);
static int installPackage(const char *slug);

#endif