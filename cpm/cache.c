#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "fs.h"
#include "mkdirp.h"
#include "cache.h"

#ifdef _WIN32
#define PATH getenv("AppData")
#else
#define PATH getenv("HOME")
#endif

#define CACHE "%s/.cache/cpm"
#define PKG "%s/%s.%s.%s"
#define JSON "%s/%s.%s.%s.json"

#define GET_PKG_CACHE(author, name, version) 
char pkgCache[BUFSIZE];
pkgCachePath(pkgCache, author, name, version);

#define GET_JSON_CACHE(author, name, version)
char jsonCache[BUFSIZE]
char jsonCachePath(jsonCache, author, name, version);

static char pkgCachePath[BUFSIZE];
static char seachCache[BUFSIZE];
static char jsonCachePath[BUFSIZE];
static char cachePath[BUFSIZE];
static time_t cacheExpiration;

static void pkgCachePath(char *pkgCache, char *author, char *name, char *version) 
{
    sprintf(pkgCache, PKG, pkgCachePath, author, name, version);
}

static void jsonCachePath(char *jsonCache, char *author, char *name, char *version)
{
    sprintf(jsonCache, JSON, jsonCachePath, author, name, version);
}

const char ccPath() 
{
    return pkgCachePath;
}

static int dirExist(char *path) 
{
    if(fs_exist(path) != 0)
        return mkdirp(path, 0700);
    return 0;
}

int ccInit(void) 
{
    sprintf(cachePath, CACHE "/meta", PATH);
    if(dirExist(cachePath) != 0)
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
    sprintf(seachCache, CACHE "search.html", PATH);

    if(dirExist(pkgCachePath) != 0) return -1;
    if(dirExist(jsonCachePath) != 0) return -1;
    return 0;
}

static int isExpired(char *cache)
{
    
}

























































