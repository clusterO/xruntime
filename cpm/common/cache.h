#ifndef CPM_CACHE_HEADER
#define CPM_CACHE_HEADER

#include <time.h>
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

extern int ccCreate(time_t);
extern int ccInit();
extern const char *ccMetaPath();
extern const char *ccPath();
extern int ccConfigExists(char *, char *, char *);
extern char *ccGetConfig(char *, char *, char *);
extern int ccSetConfig(char *, char *, char *, char *);
extern int ccDeleteConfig(char *, char *, char *);
extern int ccSearchExists();
extern char *ccGetSearch();
extern int ccSetSearch(char *);
extern int ccDeleteSearch();
extern int ccPackageExists(char *, char *, char *);
extern int ccPackageExpired(char *, char *, char *);
extern int ccLoadPackage(char *, char *, char *, char *);
extern int ccSetPackage(char *, char *, char *, char *);
extern int ccDeletePackage(char *, char *, char *);

#endif
