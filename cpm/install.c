#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <curl/curl.h>
#include "cache.h"
#include "package.h"
#include "libs/commander.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/debug.h"
#include "libs/logger.h"
#include "libs/parson.h"

#define PACKAGE_CACHE_TIME 2592000

#ifdef PTHREADS_HEADER
#define MAX_THREADS 12
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

extern CURLSH *cpcs;

debug_t debugger = {0};

struct options {
    const char *dir;
    char *prefix;
    char *token;
    int verbose;
    int dev;
    int save;
    int savedev;
    int force;
    int global;
    int skipCache;
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


static void setPrefix(command_t *self) 
{
    opts.prefix = (char *)self->arg;
    debug(&debugger, "set prefix: %s", opts.prefix);
}


static void setToken(command_t *self) 
{
    opts.token = (char *)self->arg;
    debug(&debugger, "set token: %s", opts.token);
}
static void setVerbose(command_t *self) 
{
    opts.verbose = 0;
    debug(&debugger, "unset verbose");
}
static void setDev(command_t *self) 
{
    opts.dev = 1;
    debug(&debugger, "set develmopent flag");
}
static void setSave(command_t *self) 
{
    opts.save = 1;
    debug(&debugger, "set save flag");
}

static void setSaveDev(command_t *self) 
{
    opts.savedev = 1;
    debug(&debugger, "set save develmopent flag");
}


static void setForce(command_t *self) 
{
    opts.force = 1;
    debug(&debugger, "set force flag");
}


static void setGlobal(command_t *self) 
{
    opts.global = 1;
    debug(&debugger, "set global flag");
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

static void setSkipCache(command_t *self) 
{
    opts.skipCache = 1;
    debug(&debugger, "set skip cache flag");
}

static int installLocalpkgs()
{
    const char *file = "manifest.json";

    if(fs_exists(file) == -1) {
        logger_error("error", "Missing manifest file");
        return 1;
    }

    debug(&debugger, "reading local manifest");
    char *json = fs_read(file);
    if(json == NULL) return 1;

    Package *pkg = newPkg(json, opts.verbose);
    if(pkg == NULL) goto e1;
    if(pkg->prefix) setenv("PREFIX", pkg->prefix, 1);
    
    int rc = installDeps(pkg, opts.dir, opts.verbose);
    if(rc == -1) goto e2;

    if(opts.dev) {
        rc = installDev(pkg, opts.dir, opts.verbose);
        if(rc == -1) goto e2;
    }

    free(json);
    freePkgs(pkg);
    return 0;

e2:
    freePkgs(pkg);
e1:
    free(json);
    return 1;
}
























,
