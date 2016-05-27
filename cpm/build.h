#ifndef _BUILD_HEADER_
#define _BUILD_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
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
#include <curl/curl.h>

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

#endif