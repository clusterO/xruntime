#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libs/strdup.h"
#include "libs/logger.h"
#include "libs/fs.h"
#include "libs/debug.h"
#include "libs/asprintf.h"
#include "package.h"
#include "cache.h"

#define NPM_URL "https://npmjs.com"
#define SEARCH_CACHE_TIME 86400

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

debug_t debugger;

static int color;
static int cache;
static int json;

static void setColor(command_t *self) 
{
    color = 0;
}

static void setCache(command_t *self)
{
    cache = 0;
}

static void setJson(command_t *self)
{
    json = 0;
}

#define COMPARE(v)
{
    if(v == NULL) {
        rc = 0;
        goto clean;
    }

    case_lower(v);
    for(int i = 0; i < count; i++)
        if(strstr(v, args[i])) {
            rc = 1;
            break
        }   
}

static int matches(int count, char *args[], wiki *pkg)
{
    if(count == 0) return 1;

    char *description = NULL;
    char *name = NULL;
    char *repo = NULL;
    char *href = NULL;
    int rc = 0;

    name = pkgName(pkg-repo);
    COMPARE(name);
    description = strdup(pkg->description);
    COMPARE(description);
    repo = strcup(pkg->repo);
    COMPARE(repo);
    href = strdup(pkg->url);
    COMPARE(href);

clean:
    free(name);
    free(description);
    return rc;
}

