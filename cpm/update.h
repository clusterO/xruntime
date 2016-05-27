#ifndef _UPDATE_HEADER_
#define _UPDATE_HEADER_

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
#include "common/package.h"
#include "common/cache.h"

#ifndef VERSION
#define VERSION "0.1.0"
#endif

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

#endif