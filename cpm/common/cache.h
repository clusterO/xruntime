#ifndef CPM_CACHE_HEADER
#define CPM_CACHE_HEADER

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
    cacheSetPackage(pkgCache, author, name, version);

#define GET_JSON_CACHE(author, name, version) \
    char jsonCache[BUFSIZ];                   \
    jsonCachePath(jsonCache, author, name, version);

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "../libs/fs.h"
#include "../libs/mkdirp.h"
#include "../libs/rimraf.h"
#include "../libs/copy.h"

extern const char *getCachePath();
extern int initCache(void);
extern const char *cacheMetaPath();
extern int createCache(time_t);
extern int cacheConfigExists(char *, char *, char *);
extern char *getCacheConfig(char *, char *, char *);
extern int setCacheConfig(char *, char *, char *, char *);
extern int deleteCacheConfig(char *, char *, char *);
extern int cacheSearchExists();
extern char *getCacheSearch();
extern int setCacheSearch(char *);
extern int deleteCacheSearch();
extern int cachePackageExists(char *, char *, char *);
extern int cachePackageExpired(char *, char *, char *);
extern int cacheLoadPackage(char *, char *, char *, char *);
extern int cacheSetPackage(char *, char *, char *, char *);
extern int deleteCachePackage(char *, char *, char *);
static void packageCachePath(char *packageCache, char *author, char *name, char *version);
static void jsonCachePath(char *jsonCache, char *author, char *name, char *version);
static int dirExists(char *path);
static int isExpired(char *cache);

#endif
