#ifndef _SEARCH_HEADER_
#define _SEARCH_HEADER_

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#define NPM_URL "https://npmjs.com"
#define SEARCH_CACHE_TIME 86400

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

#define COMPARE(v)                      \
    {                                   \
        if (v == NULL)                  \
        {                               \
            rc = 0;                     \
            goto clean;                 \
        }                               \
        case_lower(v);                  \
        for (int i = 0; i < count; i++) \
            if (strstr(v, args[i]))     \
            {                           \
                rc = 1;                 \
                break;                  \
            }                           \
    }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libs/strdup.h"
#include "libs/logger.h"
#include "libs/fs.h"
#include "libs/debug.h"
#include "libs/asprintf.h"
#include "libs/http-get.h"
#include "libs/console-colors.h"
#include "libs/parson.h"
#include "libs/commander.h"
#include "libs/wiki-registry.h"
#include "common/package.h"
#include "common/cache.h"

static char *cacheSearch();
static void unsetColor();
static void unsetCache();
static void setJson();
static void showPkg(const wiki_package_t *pkg, cc_color_t highlightColor, cc_color_t textColor);
static void addPkgToJson(const wiki_package_t *pkg, JSON_Array *jsonList);
static void getSearchCommandOptions(command_t *program, int argc, char **argv);
static int matches(int count, char *args[], wiki_package_t *pkg);

#endif