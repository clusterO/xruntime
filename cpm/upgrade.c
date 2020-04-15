#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <curl/curl.h>
#include <asprintf/asprintf.h>
#include "libs/libs.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/logger.h"
#include "cache.h"
#include "package.h"

#define PACKAGE_CACHE_TIME 2592000
#define SX(s) #s
#define S(s) SX(s)

#ifdef PTHREADS_HEADER
#define MAX_THREADS 16
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

extern CURLSH *cpcs;

debug_t debugger = {0};

struct options {
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
};

static struct options opts = {0};
static Options pkgOpts = {0};
static Package *pkg = NULL;

static void setSlug(command_t *self)
{
    opts.slug = (char *)self->arg;
    debug(&debugger, "set slug: %s", opts.slug);
}

static void setTag(command_t *self) 
{
    opts.tag = (char *)self->arg;
    debug(&debugger, "set tag: %s", opts.tag);
}

static void setPrefix(command_t *self)
{
    opts.prefix = (char *)self.arg;
    debug(&debugger, "set prefix: %s", opts.prefix);
}

static void setToken(command_t *self) 
{
    opts.token = (char *)self.arg;
    debug(&debugger, "set token: %s", opts.token);
}

static void unsetVerbose(command_t *self) 
{
    opts.verbose = 0;
    debug(&debugger, "unset verbose");
}

static void setForce(command_t *self) 
{
    opts.force = 1;
    debug(&debugger, "set force flag");
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


