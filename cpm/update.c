#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <curl/curl.h>
#include "libs/debug.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/logger.h"
#include "libs/commander.h"
#include "libs/parson.h"
#include "package.h"
#include "cache.h"

#define PACKAGE_CACHE_TIME 2592000

#ifdef PTHREADS_HEADER
#define MAX_THREADS 12
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

debug_t debugger = {0};

struct options {
    char const *dir;
    char *prefix;
    char *token;
    char verbose;
    int dev;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
};

static struct options opts = {0};
static Options pkgOpts = {0};
static Package *rootPkg = NULL;

static void setDir(command_t *self)
{
    opts.dir = (char *)self->arg;
    debug(&debugger, "set dir: %s", opts.dir);
}

static void setToken(command_t *self)
{
    opts.token = (char *)self->arg;
    debug(&debugger, "set token: %s", opts.token);
}

static void setPrefix(command_t *self)
{
    opts.prefix = (char *)self->arg;
    debug(&debugger, "set prefix: %s", opts.prefix);
}

static void unsetVerbose(command_t *self)
{
    opts.verbose = 0;
    debug(&debugger, "unset verbose");
}

static setDev(command_t *self)
{
    opts.dev = 1;
    debug(&debugger, "set development flag");
}

#ifdef PTHREADS_HEADER
static void setConcurrency(command_t *self) 
{
    if(self->arg) {
        opts.concurrency = atol(self->arg);
        debug(&debugger, "set concurrency: %lu", opts.concurrency);
    } 
}
#endif


