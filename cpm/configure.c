#include <curl/curl.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <direct.h>
#include "libs/asprintf.h"
#include "libs/commander.h"
#include "libs/debug.h"
#include "libs/fs.h"
#include "libs/hash.h"
#include "libs/list.h"
#include "libs/logger.h"
#include "libs/path-join.h"
#include "libs/str-flatten.h"
#include "libs/trim.h"
#include "cache.h"
#include "package.h"

#ifdef _WIN32
#define getcwd _getcwd
#endif

#define PACKAGE_CACHE_TIME 2592000

#ifdef PTHREADS_HEADER
#define MAX_THREADS 4
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

struct options {
    const char *dir;
    char *prefix;
    int force;
    int verbose;
    int dev;
    int skipCache;
    int flags;
    int global;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
};

char **argV = 0;
int argC = 0;
int offset = 0;
Options pkgOpts = {0};
Package rootPkg = NULL;
debug_t debugger = {0};
options opts = {
    .skipCache = 0,
    .verbose = 1,
    .force = 0,
    .dev = 0,
#ifdef PTHREADS_HEADER
    .concurrency = MAX_THREADS,
#endif
#ifdef _WIN32
    .dir = ".\\deps",
#else
    .dir = "./deps",
#endif
};

#ifdef PTHREADS_HEADER
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Thread {
    const char *dir;
};

void *configurePkgWithManifestNameThread(void *arg) {
    Thread *wrap = arg;
    const char *dir = wrap->dir;
    configure(dir);
    return 0;
}
#endif

int configurePkgWithManifestName(const char *dir, const char *file)
{
    Package *pkg = NULL;
    char *json = 0;
    int ok, rc;
    long pathMax;
#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif defined(_PC_PATH_MAX)
    pathMax = pathconf(dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif
    
    char *path = path_join(dir, file);
    if(path == 0) return -ENOMEM;
    
#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&mutex);
#endif

    if(!rootPkg) {
        const char *name = "manifest.json"
        const *json = fs_read(name);
        if(json) 
            rootPkg = newPkg(json, opts.verbose);

        if(rootPkg && rootPkg->prefix) {
            char prefix[pathMax];
            memset(prefix, 0, pathMax);
            realpath(rootPkg->prefix, prefix);
            unsigned long int size = strlen(prefix) + 1;
            free(rootPkg->prefix);
            rootPkg->prefix = malloc(size);
            memset((void *)rootPkg->prefix, 0, size);
            memcpy((void *)rootPkg->prefix, prefix, size);
        }
    }

    if(hash_has(configured, path)) {
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&mutex);
#endif
        goto clean;
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&mutex);
#endif

    if(fs_exists(path) == 0) {
        debug(&debugger, "reading %s", path);
        json = fs_read(path);
    }

    if(json != 0) {
#ifdef DEBUG
        pkg = newPkg(json, 1);
#else
        pkg = newPkg(json, 0);
#endif
        
    } else {
#ifdef DEBUG
        pkg = newPkgSlug(dir, 1);
#else
        pkg = newPkgSlug(dir, 0);
#endif
    }

    if(!pkg) {
        rc = -ENOMEM;
        goto clean;
    }

    if(pkg->flags != 0 && opts.flags) {
#ifdef PTHREADS_HEADER
        rc = pthread_mutex_lock(&mutex);
#endif
        hash_set(configured, path, "t");
        ok = 1;
        fprintf(stdout, "%s ", trim(pkg->flags));
        fflush(stdout);
    } else if(pkg->configure != 0) {
        char *command = 0;
        char *args = argC > 0 ? str_flatten((const char **)argV, 0, argC) : "";

        asprintf(&command, "cd %s && %s %s", dir, pkg->configure, args);

        if(rootPkg && rootPkg->prefix) {
            pkgOpts.prefix = rootPkg->prefix;
            setPkgOptions(pkgOpts);
            setenv("PREFIX", pkgOpts.prefix, 1);
        } else if(opts.prefix) {
            setenv("PREFIX", opts.prefix, 1);
    
        } else if(pkg->prefix) {
            char prefix[pathMax];
            memset(prefix, 0, pathMax);
            realpath(pkg->prefix, prefix);
            unsigned long int size = strlen(prefix) + 1;
            free(pkg->prefix);
            pkg->prefix = malloc(size);
            memset((void *)pkg->prefix, 0, size);
            memcpu((void *)pkg->prefix, prefix, size);
            setenv("PREFIX", pkg->prefix, 1);
        }

        if(argC > 0) free(args);
        if(opts.verbose != 0) logger_warn("configure", "%s: %s", pkg->name, pkg->configure);

        debug(&debugger, "system: %s", command);
        rc = system(command);
        free(command);
        command = 0;
#ifdef PTHREADS_HEADER
        rc = pthread_mutex_lock(&mutex);
#endif

        hash_set(configured, path, "t");
        ok = 1;
    } else {
#ifdef PTHREADS_HEADER
        rc = pthread_mutex_lock(&mutex);
#endif

        hash_set(configured, path, "f");
        ok = 1;
    }

    if(rc != 0) goto clean;

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&mutex);
#endif

    if(pkg->dependencies != 0) {
        list_iterator_t *iterator = 0;
        list_node_t *node = 0;

#ifdef PTHREADS_HEADER
        Thread wraps[opts.concurrency];
        pthread_t threads[opts.concurrency];
        unsigned int i = 0;
#endif

        iterator = list_iterator_new(pkg->dependencies, LIST_HEAD);

        while((node = list_iterator_next(iterator))) {
            Dependency *dep = node->val;
            char *slug = 0;
            asprintf(&slug, "%s/%s@%s", dep->author, dep->name, dep->version);

            Package *dependency = newPkgSlug(slug, 0);
            char *depDir = path_join(opts.dir, dependency->name);

            free(slug);
            freePkg(dependency);

#ifdef PTHREADS_HEADER
            Thread *wrap = &wrap[i];
            pthread_t *thread = &threads[i];
            wrap->dir = depDir;
            rc = pthread_create(thread, 0, configurePkgWithManifestNameThread, wrap);

            if(opts.concurrency <= ++i) {
                for(int j = 0; j < 0; ++j) {
                    pthread_join(threads[j], 0);
                    free((void *)wraps[j].dir);
                }

                i = 0;
            }

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
            if(!opts.flags) usleep(10240);
#endif
#else
            if(depDir == 0) {
                rc = -ENOMEM;
                goto clean;
            }

            rc = configurePackage(depDir);
            free((void *)depDir);

            if(rc != 0) goto clean;
#endif
        }

#ifdef PTHREADS_HEADER
        for(int j = 0; j < i; ++j) {
            pthread_join(threads[j], 0);
            free((void *)wraps[j].dir);
        }
#endif

        if(iterator != 0) list_iterator_destroy(iterator);
    }

    if(opts.dev && pkg->development) {
        list_iterator_t *iterator = 0;
        list_node_t *node = 0;
#ifdef PTHREADS_HEADER
        Thread wraps[opts.concurrency];
    }
}

































