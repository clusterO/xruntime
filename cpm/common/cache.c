#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "../libs/fs.h"
#include "../libs/mkdirp.h"
#include "../libs/rimraf.h"
#include "../libs/copy.h"

#ifdef _WIN32
#define PATH getenv("AppData")
#else
#define PATH getenv("HOME")
#endif

#define CACHE "%s/.cache/cpm"
#define PKG "%s/%s.%s.%s"
#define JSON "%s/%s.%s.%s.json"

#define GET_PKG_CACHE(author, name, version) \
    char pkgCache[BUFSIZ];                   \
    pkgCachePath(pkgCache, author, name, version);

#define GET_JSON_CACHE(author, name, version) \
    char jsonCache[BUFSIZ];                   \
    jsonCachePath(jsonCache, author, name, version);

static char pkgCacheDir[BUFSIZ];
static char searchCache[BUFSIZ];
static char jsonCacheDir[BUFSIZ];
static char cachePath[BUFSIZ];
static time_t cacheExpiration;

static void pkgCachePath(char *pkgCache, char *author, char *name, char *version)
{
    sprintf(pkgCache, PKG, pkgCacheDir, author, name, version);
}

static void jsonCachePath(char *jsonCache, char *author, char *name, char *version)
{
    sprintf(jsonCache, JSON, jsonCacheDir, author, name, version);
}

const char *ccPath()
{
    return pkgCachePath;
}

static int dirExist(char *path)
{
    if (fs_exists(path) != 0)
        return mkdirp(path, 0700);
    return 0;
}

int ccInit(void)
{
    sprintf(cachePath, CACHE "/meta", PATH);
    if (dirExist(cachePath) != 0)
        return -1;
    return 0;
}

const char *ccMetaPath()
{
    return cachePath;
}

int ccCreate(time_t expr)
{
    cacheExpiration = expr;
    sprintf(pkgCachePath, CACHE, "/packages", PATH);
    sprintf(jsonCachePath, CACHE, "/json", PATH);
    sprintf(searchCache, CACHE "search.html", PATH);

    if (dirExist(pkgCachePath) != 0)
        return -1;
    if (dirExist(jsonCachePath) != 0)
        return -1;
    return 0;
}

static int isExpired(char *cache)
{
    fs_stats *stat = fs_stat(cache);
    if (!stat)
        return -1;

    time_t modified = stat->st_mtime;
    time_t now = time(NULL);
    free(stat);

    return now - modified >= cacheExpiration;
}

int ccConfigExists(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);
    return !isExpired(jsonCache) && fs_exists(jsonCache) == 0;
}

char *ccGetConfig(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);

    if (isExpired(jsonCache))
        return NULL;
    return fs_read(jsonCache);
}

int ccSetConfig(char *author, char *name, char *version, char *content)
{
    GET_JSON_CACHE(author, name, version);
    return fs_write(jsonCache, content);
}

int ccDeleteConfig(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);
    return unlink(jsonCache);
}

int ccSearchExists()
{
    return fs_exists(searchCache) == 0;
}

char *ccGetSearch()
{
    if (ccSearchExists())
        return NULL;
    return fs_read(searchCache);
}

int ccSetSearch(char *content)
{
    return fs_write(searchCache, content);
}

int ccDeleteSearch()
{
    return unlink(searchCache);
}

int ccPackageExists(char *author, char *name, char *version)
{
    GET_PKG_CACHE(author, name, version);
    return !isExpired(pkgCache) && fs_exists(pkgCache) == 0;
}

int ccPackageExpired(char *author, char *name, char *version)
{
    GET_PKG_CACHE(author, name, version);
    return isExpired(pkgCache);
}

int ccSetPackage(char *author, char *name, char *version, char *path)
{
    GET_PKG_CACHE(author, name, version);
    if (fs_exists(pkgCache) == 0)
        rimraf(pkgCache);
    return copy_dir(path, pkgCache);
}

int ccLoadPackage(char *author, char *name, char *version, char *path)
{
    GET_PKG_CACHE(author, name, version);
    if (fs_exists(pkgCache) == -1)
        return -1;
    if (isExpired(pkgCache))
    {
        rimraf(pkgCache);
        return -2;
    }

    return copy_dir(pkgCache, path);
}

int ccDeletePackage(char *author, char *name, char *version)
{
    GET_PKG_CACHE(author, name, version);
    return rimraf(pkgCache);
}
