#include "cache.h"

static char packageCacheDirectory[BUFSIZ];
static char searchCacheDirectory[BUFSIZ];
static char jsonCacheDirectory[BUFSIZ];
static char cachePath[BUFSIZ];
static time_t cacheExpiration;

const char *getCachePath()
{
    return packageCachePath;
}

int initCache(void)
{
    sprintf(cachePath, CACHE "/meta", PATH);
    if (dirExists(cachePath) != 0)
        return -1;

    return 0;
}

const char *cacheMetaPath()
{
    return cachePath;
}

int createCache(time_t expr)
{
    cacheExpiration = expr;

    // sprintf require that the pointer has a fixed length beforehand
    // when its not specified it hang, what if the same behavior happen
    // when the size doesn't much, this should not hand, must treat the
    // return value for error, but avoid the hang part
    // returns the length of the written string
    sprintf(packageCacheDirectory, CACHE "/packages", PATH);
    sprintf(jsonCacheDirectory, CACHE "/json", PATH);
    sprintf(searchCacheDirectory, CACHE "/search", PATH);

    if (dirExists(packageCacheDirectory) != 0 ||
        dirExists(jsonCacheDirectory) != 0 ||
        dirExists(searchCacheDirectory) != 0)
        return -1;

    return 0;
}

int cacheConfigExists(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);
    return !isExpired(jsonCache) && fs_exists(jsonCache) == 0;
}

char *getCacheConfig(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);

    if (isExpired(jsonCache))
        return NULL;

    return fs_read(jsonCache);
}

int setCacheConfig(char *author, char *name, char *version, char *content)
{
    GET_JSON_CACHE(author, name, version);
    return fs_write(jsonCache, content);
}

int deleteCacheConfig(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);
    return unlink(jsonCache);
}

int cacheSearchExists()
{
    return fs_exists(searchCacheDirectory) == 0;
}

char *getCacheSearch()
{
    if (cacheSearchExists())
        return NULL;
    return fs_read(searchCacheDirectory);
}

int setCacheSearch(char *content)
{
    return fs_write(searchCacheDirectory, content);
}

int deleteCacheSearch()
{
    return unlink(searchCacheDirectory);
}

int cachePackageExists(char *author, char *name, char *version)
{
    GET_PKG_CACHE(author, name, version);
    return !isExpired(pkgCache) && fs_exists(pkgCache) == 0;
}

int cachePackageExpired(char *author, char *name, char *version)
{
    GET_PKG_CACHE(author, name, version);
    return isExpired(pkgCache);
}

int cacheSetPackage(char *author, char *name, char *version, char *path)
{
    GET_PKG_CACHE(author, name, version);
    if (fs_exists(pkgCache) == 0)
        rimraf(pkgCache);
    return copy_dir(path, pkgCache);
}

int cacheLoadPackage(char *author, char *name, char *version, char *path)
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

int deleteCachePackage(char *author, char *name, char *version)
{
    GET_PKG_CACHE(author, name, version);
    return rimraf(pkgCache);
}

static void packageCachePath(char *packageCache, char *author, char *name, char *version)
{
    sprintf(packageCache, PKG, packageCacheDirectory, author, name, version);
}

static void jsonCachePath(char *jsonCache, char *author, char *name, char *version)
{
    sprintf(jsonCache, JSON, jsonCacheDirectory, author, name, version);
}

static int dirExists(char *path)
{
    if (fs_exists(path) != 0)
        return mkdirp(path, 0700);

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