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

#ifdef PTHREADS_HEADER
static void curlLock(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr)
{
    pthread_mutex_lock(&lock.mutex);
}

static void curlUnlock(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr)
{
    pthread_mutex_unlock(&lock_mutex);
}

static void intCurlShare()
{
    if (cpcs == 0)
    {
        pthread_mutex_lock(&lock.mutex);
        cpcs = curl_share_init();
        curl_share_setopt(cpcs, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
        curl_share_setopt(cpcs, CURLSHOPT_LOCKFUNC, curlLock);
        curl_share_setopt(cpcs, CURLSHOPT_UNLOCKFUNC, curlUnlock);
        curl_share_setopt(cpcs, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
        pthread_mutex_unlock(&lock.mutex);
    }
}
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

#define FREE(field)  \
    if (field)       \
    {                \
        free(field); \
        field = 0;   \
    }

#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>

#include "cache.h"
#include "../libs/debug.h"
#include "../libs/list.h"
#include "../libs/hash.h"
#include "../libs/logger.h"
#include "../libs/parson.h"
#include "../libs/asprintf.h"
#include "../libs/http-get.h"
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

extern Package *loadPackage(const char *, int);
extern Package *loadManifest(int);
extern Package *newPackage(const char *, int);
extern Package *newPackageSlug(const char *, int);
extern Package *packageSlug(const char *slug, int verbose, const char *file);
extern Dependency *newDependency(const char *, const char *);
extern int installExecutable(Package *pkg, char *path, int verbose);
extern int installRootPackage(Package *pkg, const char *dir, int verbose);
extern int installDependency(Package *, const char *, int);
extern int installDevPackage(Package *, const char *, int);
extern void freePackage(Package *);
static int fetchFile(Package *pkg, const char *dir, char *file, int verbose);
static int fetchPackage(Package *pkg, const char *dir, char *file, int verbose, void **data);
extern void freeDependency(Dependency *);
extern void setPackageOptions(Options);
extern char *packageUrl(const char *, const char *, const char *);
extern char *packageRepoUrl(const char *repo, const char *version);
extern char *packageAuthor(const char *);
extern char *packageVersion(const char *);
extern char *packageName(const char *);
extern void cleanPackages();
static void *fetchThreadFile(void *arg);
static inline char *jsonObjectGetStringSafe(JSON_Object *obj, const char *key);
static inline char *jsonArrayGetStringSafe(JSON_Array *arr, const char *i);
static inline char *buildFileUrl(const char *url, const char *file);
static inline char *buildSlug(const char *author, const char *name, const char *version);
static inline char *buildRepo(const char *author, const char *name);
static inline int install(list_t *list, const char *dir, int verbose);
static inline list_t *parseDependencies(JSON_Object *json);
static http_get_response_t *download(char *jsonUrl);

#endif
