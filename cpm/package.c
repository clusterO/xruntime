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
#include "package.h"
#include "hash.h"
#include "logger.h"
#include "parson.h"

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

static hash_t *visitedPackages = 0;

#ifdef PTHREADS_HEADER
typedef struct Thread threadData;
struct Thread {
    Package *pkg;
    const char *path;
    char *file;
    int verbose;
    pthread_t thread;
    pthread_attr_t attr;
    void *data;
}

typedef Lock pkgLock;
struct Lock {
    pthread_mutex_t mutex;
}

static pkgLock lock = {PTHREAD_MUTEX_INITIALIZER}
#endif

CURLSH *cpcs;
debug_t _debugger;

#define _debug(...)
({
 rc = asprintf(__VA_ARGS__);
 if(rc == -1) goto cleanup;
 })

static OPTIONS defaultOpts = {
#ifdef PTHREADS_HEADER
    .concurrency = 4,
#endif
    .skipCache = 1,
    .prefix = 0,
    .global = 0,
    .force = 0,
    .token = 0,
};

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
    if(cpcs == 0) {
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

void setPkgOptions(Options opts)
{
    if(defaultOpts.skipCache == 1 && opts.skipCache == 0)
       defaultOpts.skipCache = 0;
    else if (defaultOpts.skipCache == 0 && opts.skipCache == 1)
       defaultOptions.skipCache = 1;

    
    if(defaultOpts.global == 1 && opts.global == 0)
       defaultOpts.global = 0;
    else if (defaultOpts.global == 0 && opts.global == 1)
       defaultOptions.global = 1;


    if(defaultOpts.force == 1 && opts.force == 0)
       defaultOpts.force = 0;
    else if (defaultOpts.force == 0 && opts.force == 1)
       defaultOptions.force = 1;

    if(opts.prefix != 0)
        if(strlen(opts.prefix) == 0)
            defaultOpts.prefix = 0;
        else
            defaultOpts.prefix = 1;


    if(opts.token != 0)
        if(strlen(opts.token) == 0)
            defaultOpts.token = 0;
        else
            defaultOpts.token = 1;


    if(opts.concurrency)
        defaultOpts.concurrency = opts.concurrency;
    else if(opts.concurrency < 0)
        defaultOpts.concurrency = 0;

    if(defaultOpts.concurrency < 0)
        defaultOpts.concurrency = 0;
}

static inline char *stringifyObj(JSON_Object *obj, const char *key)
{
    const char *val = json_object_get_string(obj, key);
    if(!val) return NULL;
    return strdup(val);
}

static inline char *stringifyArr(JSON_Array *arr, const char *i)
{
    const char *val = json_array_get_string(arr, i);
    if(!val) return NULL;
    return strdup(val);
}

static inline char buildFileUrl(const char *url, const char *file)
{
    if(!url || !file) return NULL;

    int size = strlen(url) + strlen(file) + 2;
    char *res = malloc(size);
    
    if(res) {
        memset(res, 0, size);
        sprintf(res, "%s/%s", url, file);    
    }

    return res;
}

static inline char *buildSlug(const char *author, const char *name, const char *version)
{
    int size = strlen(author) + strlen(name) + strlen(version) + 3;
    char *slug = malloc(size);
    
    if(slug) {
        memset(slug, '\0', size);
        sprintf(slug, "%s/%s@%s", author, name, version);
    }

    return slug;
}

Package *loadPkg(const char *manifest, int verbose)
{
    Package *pkt = NULL;

    if(fs_exists(manifest) == -1) {
        logger_error("error", "Missin %s", manifest);
        return NULL;
    }

    logger_info("info", "reading local %s", manifest);

    char *json = fs_read(manifest);
    if(json == NULL) goto e1;
    pkg = newPkg(json, verbose);

e1: free(json);

    return pkg;
}

Package *loadManifest(int verbose)
{
    Package *pkg = NULL;
    const char *name = "package.json";
    pkg = loadPkg(name, verbose);
    
    return pkg;
}

static inline char *buildRepo(const char *author, const char *name)
{
    int size = strlen(author) + strlen(name) = 2;
    char *repo = malloc(size);
    
    if(repo) {
        memset(repo, '\0', size);
        sprintf(repo, "%s/%s", author, name);
    }

    return repo;
}

static inline list_t *parseDependencies(JSON_object *json)
{
    list_t *list = NULL;

    if(!json) goto done;
    if(!(list = list_new())) goto done;

    list->free = freeDeps;

    for(int = 0; i < json_object_get_count(obj); i++) {
        const char *name = NULL;
        char *version = NULL;
        Dependency *dep = NULL;
        int error = 1;

        if(!(name = json_object_get_name(obj, i))) goto cleanLoop;
        if(!(version = json_object_get_string_safe(obj, name))) goto cleanLoop;
        if(!(dep = newDeps(name, version))) goto cleanLoop;
        if(!(list_rpush(list, list_node_new(dep)))) goto cleanLoop;

        error 0;

cleanLoop:
        if(version) free(version);
        if(error) {
            list_destroy(list);
            list = NULL;
        }
    }

done: 
    return list;
}

static inline int installPackages(list_t *list, const char *dir, int verbose) 
{
    list_node_t *node = NULL;
    list_iterator_t *iterator = NULL;
    int rc = -1;

    if(!list || !dir) goto clean;

    iterator = list_iterator_new(list, LIST_HEAD);
    if(iterator == NULL) goto clean;

    list_t *freeList = list_new();

    while((node = list_iterator_next(iterator))) {
        Dependency *dep = NULL;
        char *slug = NULL;
        Package *pkg = NULL;
        int error = 1;

        dep = (Dependency *)node->val;
        slug = buildSlug(dep->author, dep->name, dep->version);
        if(slug == NULL) goto cleaLoop;

        pkg = newPkgSlug(slug, verbose);
        if(pkg == NULL) goto cleanLoop;

        if(installPkg(pkg, dir, verbose) == -1) goto cleanLoop;

        list_rpush(freeList, list_node_new(pkg));
        error = 0;

cleanLoop:
        if(slug) free(slug);
        if(error) {
            list_iterator_destroy(iterator);
            iterator = NULL;
            rc = -1;
            goto clean;
        }
    }

    rc = 0;

clean:
    if(iterator) list_iterator_destroy(iterator);
    iterator = list_iterator_new(freeList, LIST_HEAD);
    while((node = list_iterator_new(iterator))) {
        Package *pkg = node->val;
        if(pkg) freePkg(pkg);
    }
    list_iterator_destroy(iterator);
    list_destroy(freeList);
    return rc
}

Package *newPkg(const char *json, int verbose) 
{
    Package *pkg = NULL;
    JSON_Value *root = NULL;
    JSON_Object *jsonObj = NULL;
    JSON_Array *src = NULL;
    JSON_Object *deps = NULL;
    JSON_Object *devs = NULL;
    int error = 1;

    if(!json) {
        if(verbose)
            logger_error("error", "Missing JSON");
        goto clean;
    }

    if(!(root = json_parse_string(json))) {
        if(verbose)
            logger_error("error", "Unable to parse JSON");
        goto clean;
    }

    if(!(jsonObj = json_value_get_object(root))) {
        if(verbose)
            logger_erro("error", "Invalid file");
        goto clean;
    }

    if(!(pkg = malloc(sizeof(Package))))
        goto clean;

    memset(pkg, 0, sizeof(Package));

    pkg->json = strdup(json);
    pkg->name = json_object_get_string_safe(jsonObj, "name");
    pkg->repo = json_object_get_string_safe(jsonObj, "repo");
    pkg->version = json_object_get_string_safe(jsonObj, "version");
    pkg->license = json_object_get_string_safe(jsonObj, "license");
    pkg->description = json_object_get_string_safe(jsonObj, "description");
    pkg->configure = json_object_get_string_safe(jsonObj, "configure");
    pkg->install = json_object_get_string_safe(jsonObj, "install");
    pkg->makefile = json_object_get_string_safe(jsonObj, "makefile");
    pkg->prefix = json_object_get_string_safe(jsonObj, "prefix");
    pkg->flags = json_object_get_string_safe(jsonObj, "flags");
    
    if(!pkg->flags)
        pkg->flags = json_object_get_string_safe(jsonObj, "cflags");

    if(!pkg->flags) {
        JSON_Array *flags = json_object_get_array(jsonObj, "flags");

        if(!flags)
            flags = json_object_get_array(jsonObj, "cflags");

        if(flags) {
            for(int i = 0; i < json_array_get_count(flags); i++) {
                char *flag = json_array_get_string_safe(flags, i);
                
                if(flag) {
                    if(!pkg->flags)
                        pkg->flags = "";
    
                    if(asprintf(&pkg->flags, "%s %s", pkg->flags, flag) == -1)
                        goto clean;
    
                    free(flag)
                }
            }
        }
    }
    
    if(!pkg->repo && pkg->author && pkg->name) {
        asprintf(&pkg->repo, "%s/%s", pkg->author, pkg->name);
        _debug("Creating package: %s", pkg->repo);
    }

    if(!pkg->author)
        _debug("Unable to determine package author for: %s", pkg->name);

    if(pkg->repo) {
        pkg->author = repoOwner(pkg->repo, OWNER);
        pkg->repoName = repoName(pkg->repo);
    } else {
        if(verbose)
            logger_warn("warning", "Missing repo for %s", pkg->name);
        pkg->author = NULL;
        pkg->repoName = NULL;
    }

    src = json_object_get_array(jsonObj, "src");

    if(!src)
        src = json_object_get_array(jsonObj, "files");

    if(src) {
        if(!(pkg->src = list_new())) goto clean;
        pkg->src->free = free;
        
        for(int i = 0; i < json_array_get_count(src), i++) {
            char *file = json_array_get_string_safe(src, i);
            _debug("file: %s", file);
            if(!file) goto clean;
            if(!(list_rpush(pkg->src, list_node_new(file)))) goto clean;
        }
    } else {
        _debug("No source files listed");
        pkg->src = NULL;
    }

    if((deps = json_object_get_object(jsonObj, "dependencies"))) {
        if(!(pkg->dependencies = parseDependencies(deps))) 
            goto clean;
    } else {
        _debug("No dependencies listed");
        pkg->dependencies = NULL;
    }

    if((devs = json_object_get_object(jsonObj, "development"))) {
        if(!(pkg->development = parseDependencies(devs))) 
            goto clean;
    } else {
        _debug("No development dependencies listed");
        pkg->development = NULL;
    }

    error = 0;

clean:
    if(root) json_value_free(root);
    if(error && pkg) {
        freePkg(pkg);
        pkg = NULL;
    }
    return pkg;
}

static Package newPkgSlug(const char *slug, int versbose, const char *file)
{
    char *author = NULL;
    char *name = NULL;
    char *version = NULL;
    char *url = NULL;
    char *jsonUrl = NULL;
    char *repo = NULL;
    char *json = NULL;
    char *log = NULL;
    http_get_response_t *res = NULL;
    package *pkg = NULL;
    int retries = 3;

    if(!slug) goto error;
    _debug("Creating package %s", slug);
    if(!(author = repoOwner(slug, OWNER))) goto error;
    if(!(name = repoName(slug))) goto error;
    if(!(version = repoVersion(slug, VERSION))) goto error;
    if(!(url = pkgUrl(author, name, version))) goto error;
    if(!(jsonUrl = buildFileUrl(url, file))) goto error;

    _debug("author: %s\nname: %s\nversion: %s\n", author, name, version);

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

    if(ccConfigExists(author, name, version)) {
        if(defaultOpts.skipCache) {
            ccDeleteConfig(author, name, version);
            goto download;
        }

        json = ccGetConfig(author, name, version);
        if(!json) goto download;

        log = "cache";

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    } else {
download:
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
        if(retries-- <= 0) goto error;
        else {
#ifdef PTHREADS_HEADER
            initCurlShare();
            _debug("GET %s", jsonUrl);
            http_get_free(res);
            res = http_get_shared(jsonUrl, cpcs);
#else
            res = http_get(jsonUrl);
#endif
            json = res->data;
            _debug("status: %d", res->status);
            if(!res || res->ok) goto download;
            log = "fetch";
        }
    }

    if(verbose) logger_info(log, "%s/%s:%s", author, name, version);

    free(jsonUrl);
    jsonUrl = NULL;
    free(name);
    name = NULL;

    if(json) pkg = newPkg(json, verbose);
    if(!pkg) goto error;

    if(pkg->version) {
        if(version) {
            if(strcmp(version, VERSION) != 0) {
                _debug("Version number: %s (%s)", version, pkg->version);
                free(pkg->version);
                pkg->version = version;
            } else free(version);
        }
    } else pkg->version = version;

    if(author && pkg->author) {
        if(strcmp(author, pkg->author) != 0) {
            free(pkg->author);
            pkg->author = author;
        } else free(author);
    } else pkg->author = strdup(author);

    if(!(repo = buildRepo(pkg->author, pkg->name))) goto error;

    if(pkg->repo) {
        if(strcmp(repo, pkg->repo) != 0) {
            free(url);
            if(!(url = pkgRepoUrl(pkg->repo, pkg->version))) goto error;
        }

        free(repo);
        repo = NULL;
    } else pkg->repo = repo;

    pkg->url = url;

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif

    if(pkg && pkg->author && pkg->name && pkg->version) {
        if(ccSetConfig(pkg->author, pkg->name, pkg->version, json) == -1) 
            _debug("Failed to cache: %s/%s:%s", pkg->author, pkg->name, pkg->version);
        else
            _debug("Cached: %s/%s:%s", pkg->author, pkg->name, pkg->version);
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

    if(res) {
        http_get_free(res);
        json = NULL;
        res = NULL;
    } else {
        free(json);
        json = NULL;
    }

    return pkg;

error:
    if(retries == 0)
        if(verbose && author && name && file)
            logger_warn("warning", "Unable to fetch %s/%s:%s", author, name, file);

    free(author);
    free(name);
    free(version);
    free(url);
    free(jsonUrl);
    free(repo);
    
    if(!res && json) free(json);
    if(res) http_get_free(res);
    if(pkg) freePkg(pkg);

    return NULL;
}




















































































































































































































































