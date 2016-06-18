#ifndef CPM_PACKAGE_HEADER
#define CPM_PACKAGE_HEADER

#include <curl/curl.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "../libs/list.h"
#include "../libs/hash.h"
#include "../libs/logger.h"
#include "../libs/parson.h"
#include "../libs/asprintf.h"
#include "../libs/http-get.h"
#include "../libs/debug.h"
#include "../libs/path-join.h"
#include "../libs/parse-repo.h"
#include "../libs/tempdir.h"
#include "cache.h"

#ifndef VERSION
#define VERSION "master"
#endif

#ifndef OWNER
#define OWNER "cpm"
#endif

#define GITHUB_RAW_CONTENT_URL "https://raw.githubusercontent.com/"
#define GITHUB_RAW_CONTENT_AUTH_URL "https://%s@raw.githubusercontent.com/"

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(fn, rn) _fullpath(fn, rn, strlen(fn))
#endif

typedef struct Thread Thread;
struct Thread
{
    Package *pkg;
    const char *dir;
    char *file;
    int verbose;
    pthread_t thread;
    pthread_attr_t attr;
    void *data;
};

typedef struct Lock Lock;
struct Lock
{
    pthread_mutex_t mutex;
};

typedef struct
{
    char *name;
    char *author;
    char *version;
} Dependency;

typedef struct
{
    char *name;
    char *author;
    char *description;
    char *install;
    char *configure;
    char *json;
    char *license;
    char *repo;
    char *reponame;
    char *url;
    char *version;
    char *makefile;
    char *filename;
    char *flags;
    char *prefix;
    list_t *dependencies;
    list_t *development;
    list_t *src;
    void *user;
    unsigned int refs;
} Package;

typedef struct
{
    int skipCache;
    int force;
    int global;
    char *prefix;
    int concurrency;
    char *token;
} Options;

CURLSH *cpcs;

extern void setPkgOptions(Options);
extern Package *newPkg(const char *, int);
extern Package *newPkgSlug(const char *, int);
extern Package *loadPkg(const char *, int);
extern Package *loadManifest(int);
extern char *pkgUrl(const char *, const char *, const char *);
extern char *pkgRepoUrl(const char *repo, const char *version);
extern char *pkgVersion(const char *);
extern char *pkgAuthor(const char *);
extern char *pkgName(const char *);
extern Dependency *newDeps(const char *, const char *);
extern int installExe(Package *pkg, char *path, int verbose);
extern int installPkg(Package *, const char *, int);
extern int installDeps(Package *, const char *, int);
extern int installDev(Package *, const char *, int);
extern void freePkg(Package *);
extern void freeDeps(Dependency *);
extern void cleanPkgs();

#endif
