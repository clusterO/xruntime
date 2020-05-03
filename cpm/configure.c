#include <curl/curl.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <direct.h>
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
#include "cache.h"
#include "package.h"

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
    int flags;
    int global;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
};

Options pkgOpts = {0};
Package rootPkg = NULL;

debug_t debugger = {0};

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

#ifdef PTHREADS_HEADER
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Thread {
    const char *dir;
};

void *configurePkgThread(void *arg) {
    Thread *wrap = arg;
    const char *dir = wrap->dir;
    configure(dir);
    return 0;
}
#endif



































