#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cache.h"

#ifdef _WIN32
#define BASE_DIR getenv("AppData")
#else
#define BASE_DIR getenv("HOME")
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

static char pkgCacheDir[BUFSIZE];
static char seachCache[BUFSIZE];
static char jsonCacheDir[BUFSIZE];
static char cacheDir[BUFSIZE];
static time_t cacheExpiration;

static void pkgCachePath(char *pkgCache, char *author, char *name, char *version) 
{
    sprintf(pkgCache, PKG, pkgCacheDir, author, name, version);
}

static void jsonCachePath(char *jsonCache, char *author, char *name, char *version)
{
    sprintf(jsonCache, JSON, jsonCacheDir, author, name, version);
}

