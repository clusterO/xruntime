#ifndef CPM_PACKAGE_HEADER
#define CPM_PACKAGE_HEADER

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

#define _debug(...)                            \
    ({                                         \
        if (!(_debugger.name))                 \
            debug_init(&_debugger, "package"); \
        debug(&_debugger, __VA_ARGS__);        \
    })

#define E_FORMAT(...)               \
    ({                              \
        rc = asprintf(__VA_ARGS__); \
        if (rc == -1)               \
            goto clean;             \
    })

#include <curl/curl.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "cache.h"
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
    Package *pkg;
    const char *dir;
    char *file;
    int verbose;
    pthread_t thread;
    pthread_attr_t attr;
    void *data;
} Thread;

typedef struct
{
    pthread_mutex_t mutex;
} Lock;

typedef struct
{
    char *name;
    char *author;
    char *version;
} Dependency;

typedef struct
{
    int skipCache;
    int force;
    int global;
    char *prefix;
    int concurrency;
    char *token;
} Options;

static inline char *json_object_get_string_safe(JSON_Object *obj, const char *key);
static inline char *json_array_get_string_safe(JSON_Array *arr, const char *i);
static inline char buildFileUrl(const char *url, const char *file);
static inline char *buildSlug(const char *author, const char *name, const char *version);
static inline char *buildRepo(const char *author, const char *name);
static inline list_t *parseDependencies(JSON_Object *json);
static inline int install(list_t *list, const char *dir, int verbose);
static int fetchFile(Package *pkg, const char *dir, char *file, int verbose);
static void *fetchThreadFile(void *arg);
static int fetchPkg(Package *pkg, const char *dir, char *file, int verbose, void **data);
static Package *pkgSlug(const char *slug, int verbose, const char *file);

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
