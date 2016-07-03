#ifndef _INSTALL_HEADER_
#define _INSTALL_HEADER_

#ifndef VERSION
#define VERSION "1.0.0"
#endif

#ifdef PTHREADS_HEADER
#define MAX_THREADS 12
#endif

#define PACKAGE_CACHE_TIME 2592000

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

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
#include <libgen.h>
#include <limits.h>
#include <curl/curl.h>

#include "libs/commander.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/debug.h"
#include "libs/logger.h"
#include "libs/parson.h"
#include "common/cache.h"
#include "common/package.h"

typedef struct
{
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
} InstallOptions;

static void setDir(command_t *self);
static void setPrefix(command_t *self);
static void setToken(command_t *self);
static void unsetVerbose(command_t *self);
static void setDev(command_t *self);
static void setSave(command_t *self);
static void setSaveDev(command_t *self);
static void setForce(command_t *self);
static void setSkipCache(command_t *self);
static void getInstallCommandOptions(command_t *program, int argc, char **argv);
static int writeDependency(Package *pkg, char *prefix);
static int saveDependency(Package *pkg);
static int saveDevDependency(Package *pkg);
static int installLocalPackages();
static int installPackage(const char *slug);
static int installPackages(int n, char **pkgs);
void setDir(command_t *self);
void setPrefix(command_t *self);
void setToken(command_t *self);
void unsetVerbose(command_t *self);
void setDev(command_t *self);
void setGlobal(command_t *self);

#endif