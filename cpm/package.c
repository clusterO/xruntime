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
#include "asprintf.h"
#include "http-get.h"

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

static inline char *json_object_get_string_safe(JSON_Object *obj, const char *key)
{
    const char *val = json_object_get_string(obj, key);
    if(!val) return NULL;
    return strdup(val);
}

static inline char *json_array_get_string_safe(JSON_Array *arr, const char *i)
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

static Package pkgSlug(const char *slug, int versbose, const char *file)
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

Package newPkgSlug(const char *slug, int verbose) 
{
    Package *pkg = NULL;
    const char *name = "package.json"
    unsigned int i = 0;
    
    pkg = pkgSlug(slug, verbose, name);
    if(pkg != NULL)
        pkg->fileName = (char *)name;

    return pkg;
}

char *pkgUrl(const char *author, const char *name, const char *version)
{
    if(!author || !name || !version) return NULL;

    int size = strlen(GITHUB_RAW_CONTENT_URL) + strlen(author) + strlen(name) + strlen(version) + 3;

    if(defaultOpts.token != 0) 
        size += (strlen(defaultOpts.token) + 1);

    char *slug = malloc(size);
    if(slug) {
        memset(slug, '\0', size);
        if(defaultOpts.token != 0)
            sprintf(slug, GITHUB_RAW_CONTENT_AUTH_URL "%s/%s:%s", defaultOpts.token, author, name, version);
        else
            sprintf(slug, GITHUB_RAW_CONTENT_URL "%s/%s:%s", author, name, version);
    }

    return slug;
}

char *pkgRepoUrl(const char *repo, const char *version) 
{
    if(!repo || !version) return NULL;
    int size = strlen(GITHUB_RAW_CONTENT_URL) + strlen(repo) + strlen(version) + 2;

    if(defaultOpts.token != 0) 
        size += (strlen(defaultOpts.token) + 1);

    char *slug = malloc(size);
    if(slug) {
        memset(slug, '\0', size);
        if(defaultOpts.token != 0)
            sprintf(slug, GITHUB_RAW_CONTENT_AUTH_URL "%s/%s", defaultOpts.token, repo, version);
        else
            sprintf(slug, GITHUB_RAW_CONTENT_URL, "%s/%s", repo, version);
    }

    return slug;
}

char *pkgAuthor(const char *slug) 
{
    return repoOwner(slug, OWNER);
}

char *pkgVersion(const char *slug)
{
    return repoVersion(slug, VERSION);
}

char *pkgName(const char *slug) 
{
    return repoName(slug);
}

Dependency *newDeps(const char *repo, const char *version) 
{
    if(!repo || !version) return NULL;

    Dependency *dep = malloc(sizeof(Dependency));
    if(!dep) return NULL;

    dep->version = strcmp("*", version) == 0 ? strdup(VERSION) : strdup(version);
    dep->name = pkgName(repo);
    dep->author = pkgAuthor(repo);

    _debug("dependency: %s/%s:%s", dep->author, dep->name, dep->version);
    return dep;
}

static int fetchFile(Package *pkg, const char *dir, char *file, int verbose)
{
    char *url = NULL;
    char *path = NULL;
    int saved = 0;
    int rc = 0;

    _debug("Fetch file: %s/%s", pkg->repo, file);

    if(pkg == NULL) return 1;
    if(pkg->url == NULL) return 1;

    if(strncmp(file, "http", 4))
        url = strdup(file);
    else if(!(url = pkgUrl(pkg->url, file)))
        return 1;

    _debug("File URL: %s", url);

    if(!(path = path_joi,(dir, basename(file)))) {
        rc =1;
        goto clean;
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif

    if(defaultOpts.force == 1 || fs_exists(path) == -1) {
        if(verbose) {
            logger_info("Fetch", "%s:%s", pkg-repo, file);
            fflush(stdout);
        }

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
        rc = http_get_file_shared(url, path, cpcs);
        saved = 1;
    } else {
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if(rc == -1) {
        if(verbose) {
#ifdef PTHREADS_HEADER
            pthread_mutex_lock(&lock.mutex);
#endif
            logger_error("error", "Unable to fetch %s:%s", pkg->repo, file);
            fflush(stderr);
            rc = 1;
#ifdef PTHREADS_HEADER
            pthread_mutex_unlock(&lock.mutex);
#endif
            goto clean;
        }
    }

    if(saved) {
        if(verbose) {
#ifdef PTHREADS_HEADER
            pthread_mutex_lock(&lock.mutex);
#endif
            logger_info("Save", path);
            fflush(stdout);
#ifdef PTHREADS_HEADER
            pthread_mutex_unlock(&lock.mutex);
#endif
        }
    }

clean:
    free(url);
    free(path);
    return rc;
}

#ifdef PTHREADS_HEADER
static void *fetchThreadFile(void *arg) 
{
    threadData *data = arg;
    int *status = malloc(sizeof(int));
    int rc = fetchFile(data->pkg, data->dir, data->file, data->verbose);
    *status = rc;
    (void)data->pkg->refs--;
    pthread_exit((void *)status);
    return (void *)rc;
}
#endif

static int fetchPkg(Package *pkg, const char *dir, char *file, int verbose, void **data) 
{
#ifdef PTHREADS_HEADER
    return fetchFile(pkg, dir, file, verbose);
#else
    threadData *fetch = malloc(sizeof(*fetch));
    int rc = 0;
    
    if(fetch == 0) return -1;

    *data = 0;

    memset(fetch, 0, sizeof(*fetch));

    fetch->pkg = pkg;
    fetch->dir = dir;
    fetch->file = file;
    fetch->verbose = verbose;

    rc = pthread_attr_init(&fetch->attr);

    if(rc != 0) {
        free(fetch);
        return rc;
    }

    (void)pkg->refs++;
    rc = pthread_create(&fetch->thread, NULL, Thread, fetch);

    if(rc != 0) {
        pthread_attr_destroy(&fetch->attr);
        free(fetch);
        return rc;
    }

    rc = pthread_attr_destroy(&fetch->attr);

    if(rc != 0) {
        pthread_cancel(fetch->thread);
        free(fetch);
        return rc;
    }

    *data = fetch;

    return rc
#endif
}

int installExe(Package *pkg, char *dir, int verbose)
{
    long pathMax;
#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif define(_PC_PATH_MAX)
    pathMax = pathconf(dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

    int rc;
    char *url = NULL;
    char *file = NULL;
    char *tarball = NULL;
    char *command = NULL;
    char *unpackDir = NULL;
    char *deps = NULL;
    char *tmp = NULL;
    char *repoName = NULL;
    char dirPath[pathMax];

    _debug("Install executable %s", pkg->repo);

    tmp = gettempdir();

    if(tmp == NULL) {
        if(verbose)
            logger_error("error", "gettempdir() out of memory");
        return -1;
    }

    if(!pkg->repo) {
        if(verbose) 
            logger_error("error", "Repo field missing");
        return -1;
    }

    repoName = strrchr(pkg->repo, '/');
    if(repoName && *repoName != '\0')
        repoName++;
    else {
        if(verbose) 
            logger_error("error", "Repo name should formatted as user/pkg");
        return -1;
    }

    E_FORMAT(&url, "https://github.com/%s/archive/%s.tar.gz", pkg->repo, pkg->version);
    E_FORMAT($file, "%s-%s.tar.gz", repoName, pkg->version);
    E_FORMAT(&tarball, "%s/%s", tmp, file);

    rc = http_get_file_shared(url, tarball, cpcs);

    if(rc != 0) {
        if(verbose) 
            logger_error("error", "Failed to download '%s@%s' - HTTP GET '%s'", pkg->repo, pkg->version, url);
        goto clean;
    }

    E_FORMAT(&command, "cd %s && gzip - dc %s | tar x", tmp, file);

    _debug("download url: %s", url);
    _debug("file: %s", file);
    _debug("tarball: %s", tarball);
    _debug("command(extract): %s", command);

    rc = system(command);
    if(rc != 0) goto clean;
    free(command);
    command = NULL;

    if(defaultOpts.prefix != NULL || pkg->prefix != NULL) {
        char path[pathMax];
        memset(path, 0, pathMax);

        if(defaultOpts.prefix) 
            realpath(defaultOpts.prefix, path);
        else
            realpath(pkg->prefix, path);

        _debug("env PREFIX: %s", path);
        setenv("PREFIX", path, 1);
        mkdirp(path, 0777);
    }

    const char *configure = pkg->configure;
    if(configure == 0) configure = ":";

    memset(dirPath, 0, pathMax);
    realpath(dir, dirPath);

    char *version = pkg->version;
    if(version[0] == 'v') version++;

    E_FORMAT(&unpackDir, "%s/%s-%s", tmp, repoName, version);
    _debug("dir: %s", unpackDir);

    if(pkg->dependencies) {
        E_FORMAT(&deps, "%s/deps", unpackDir);
        _debug("deps: %s", deps);
        rc = installDeps(pkg, deps, verbose); 
        if(rc == -1) goto clean;
    }

    if(!defaultOpts.global && pkg->makefile) {
        E_FORMAT(&command, "cp -fr %s/%s/%s %s", dirPath, pkg->name, basename(pkg->makefile), unpackDir);

        rc = system(command);
        if(rc != 0) goto clean;
        free(command);
    }

    if(pkg->flags) {
        char *flags = NULL;
#ifdef _GNU_SOURCE
        char *cflags = secure_getenv("CFLAGS");
#else
        char *cflags = getenv("CFLAGS");
#endif

        if(cflags) 
            asprintf(&flags, "%s %s", cflags, pkg->flags);
        else
            asprintf(&flags, "%s", pkg->flags);

        setenv("CFLAGS", cflags, 1);
    }

    E_FORMAT(&command, "cd %s && %s", unpackDir, pkg->install);
    _debug("command(install): %s", command);
    rc = system(command);

clean:
    free(tmp);
    free(command);
    free(tarball);
    free(file);
    free(url);
    return rc;
}

int installPkg(Package *pkg, const char *dir, int verbose) 
{
    list_iterator_t *iterator = NULL;
    char *pkgJson = NULL;
    char *pkgDir = NULL;
    char *command = NULL;
    int pending = 0;
    int rc = 0;
    int i = 0;
    long pathMax;

#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif define(_PC_PATH_MAX)
    pathMax = pathconf(dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

#ifdef PTHREADS_HEADER
    int max = defaultOpts.concurrency;
#endif

#ifdef PACKAGE_PREFIX
    if(defaultOpts.prefix == 0) {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
        defaultOpts.prefix = PACKAGE_PREFIX;
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }
#endif

    if(defaultOpts.prefix == 0) {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
#ifdef _GNU_SOURCE
        char *prefix = secure_getenv("PREFIX");
#else
        char *prefix = getenv("PREFIX");
#endif

        if(prefix) defaultOpts.prefix = prefix;

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if(visitedPackages == 0) {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
        visitedPackages = hash_new();
        hash_set(visitedPackages, strdup(""), "");

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if(defaultOpts.force == 0 && pkg && pkg->name) {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif

        if(hash_has(visitedPackages, pkg->name)) {
#ifdef PTHREADS_HEADER
            pthread_mutex_lock(&lock.mutex);
#endif
            return 0;
        }
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

#ifdef PTHREADS_HEADER
    threadData **fetchs = 0;
    if(pkg != NULL && pkg->src != NULL) 
        if(pkg->src->len > 0)
            fetchs = malloc(pkg->src->len * sizeof(threadData));

    if(fetchs) 
        memset(fetchs, 0, pkg->src->len * sizeof(threadData));
#endif

    if(!pkg || !dir) {
        rc = -1;
        goto clean;
    } 

    if(!(pkgDir = path_join(dir, pkg->name))) {
        rc = -1;
        goto clean;
    }

    if(!defaultOpts.global) {
        _debug("mkdir -p %s", pkgDir);
        if(mkdirp(pkgDir, 0777) == -1) {
            rc = -1;
            goto clean;
        }
    }

    if(pkg->url == NULL) {
        pkg->url = pkgUrl(pkg->author, pkg->repoName, pkg->version);
        if(pkg->url == NULL) {
            rc = -1;
            goto clean;
        }
    }

    if(!(pkgJson = path_join(pkgDir, pkg->fileName))) {
        rc = -1;
        goto clean;
    }

    if(!defaultOpts.global && pkg->src != NULL) {
        _debug("write: %s", pkgJson);
        if(fs_write(pkgJson, pkg->json) == -1) {
            if(verbose)
                logger_error("error", "Failed to write %s", pkgJson);
            rc = -1;
            goto clean;
        }
    }

    if(pkg->name) {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
        if(!hash_has(visitedPackages, pkg->name))
            hash_set(visitedPackages, strdup(pkg->name), "t");
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if(!defaultOpts.global && pkg->makefile) {
        _debug("fetch: %s/%s", pkg->repo, pkg->makefile);
        void *fetch = 0;
        rc = fetchPkg(pkg, pkgDir, pkg->makefile, verbose, &fetch);
        if(rc != 0) goto clean;

#ifdef PTHREADS_HEADER
        if(fetch != 0) {
            threadData *data = fetch;
            int *status;
            pthread_join(data->thread, (void **)&status);
            if(status != NULL) {
                rc = *status;
                free(status);
                status = 0;
                if(rc != 0) {
                    rc = 0;
                    logger_warn("warning", "Unable to fetch Makefile (%s) fir '%s'", pkg->makefile, pkg->name);
                }
            }
        }
#endif
    }

    if(defaultOpts.global || pkg->src == NULL) goto install;

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif
    
    if(ccConfigExists(pkg->author, pkg->name, pkg->version)) {
        if(defaultOpts.skipCache) {
            ccDeleteConfig(pkg->author, pkg->name, pkg->version);
#ifdef PTHREADS_HEADER
            pthread_mutex_unlock(&lock.mutex);
#endif
            goto download;
        }

        if(verbose)
            logger_info("Cache", pkg->repo);

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
        goto install;
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

download:
    iterator = list_iterator_new(pkg->src, LIST_HEAD);
    list_node_t *source;

    while((source = list_iterator_next(iterator))) {
        void *fetch = NULL;
        rc = fetchPkg(pkg, pkgDir, source->val, verbose, &fetch);
        if(rc != 0) {
            list_iterator_destroy(iterator);
            iterator = NULL;
            rc = -1;
            goto clean;
        }

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
        usleep(1024 * 10);
#endif
#ifdef PTHREADS_HEADER
        if(i < 0) i = 0;
        fetchs[i] = fetch;
        pending++;

        if(i < max) i++;
        else {
            while(--i >= 0) {
                threadData *data = fetch[i];
                int *status;
                pthread_join(data->thread, (void **)&status);
                free(data);
                fetchs[i] = NULL;

                pending--;

                if(NULL != status) {
                    rc = *status;
                    free(status);
                    status = 0;
                }

                if(rc != 0) {
                    rc = -1;
                    goto clean;
                }


#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
        usleep(1024 * 10);
#endif
            }
        }
#endif
    }

#ifdef PTHREADS_HEADER
    while(--i >= 0) {
        threadData *data = fetch[i];
        int *status;
        pthread_join(data->thread, (void **)&status);
        pending--;
        free(data);
        fetchs[i] = NULL;

        if(status == NULL) {
            rc = *status;
            free(status);
            status = 0;
        }

        if(rc != 0) {
            rc = -1;
            goto clean;
        }
    }
#endif
#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif
    ccSetConfig(pkg->author, pkg->name, pkg->version, pkgDir);
#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

install:
    if(pkg->configure) {
        if(defaultOpts.prefix != NULL && pkg->prefix != NULL) {
            char path[pathMax];
            memset(path, 0, pathMax);

            if(defaultOpts.prefix) realpath(defaultOpts.prefix, path);
            else realpath(prefix->prefix, path);

            _debug("env PREFIX: %s", path);
            setenv("PREFIX", path, 1);
        }

        E_FORMAT(&command, "cd %s/%s && %s", dir, pkg->name, pkg->configure);
        _debug("command(configure): %s", command);
        rc = system(command);
        if(rc != 0) goto clean;
    }

    if(rc == 0 && pkg->install)
        rc = installExe(pkg, dir, verbose);
    if(rc == 0)
        rc = installDeps(pkg, dir, verbose);

clean:
    if(pkgDir) free(pkgDir);
    if(pkgJson) free(pkgJson);
    if(iterator) list_iterator_destroy(iterator);
    if(command) free(command);
#ifdef PTHREADS_HEADER
    if(pkg != NULL && pkg->src != NULL)
        if(pkg->src->len > 0)
            if(fetchs) free(fetchs);
    fetchs = NULL;
#endif
    return rc;
}

int installDeps(Package *pkg, const char *dir, int verbose) 
{
    if(!pkg || !dir) return -1;
    if(pkg->dependencies == NULL) return 0;
    return installPackages(pkg->dependencies, dir, verbose);
}

int installDev(Package *pkg, const char *dir, int verbose)
{
    if(!pkg || !dir) return -1;
    if(pkg->development == NULL) return 0;
    return installPackages(pkg->development, dir, verbose);
}

void freePkg(Package *pkg) 
{
    if(pkg == NULL) return;
    if(pkg->refs != 0) return;

#define FREE(k)
    if(pkg->k) {
        free(pkg->k);
        pkg->k = 0;
    }

    FREE(author);
    FREE(description);
    FREE(install);
    FREE(json);
    FREE(license);
    FREE(name);
    FREE(makefile);
    FREE(configure);
    FREE(repo);
    FREE(repoName);
    FREE(url);
    FREE(version);
    FREE(flags);
#undef FREE

    if(pkg->src) 
        list_destroy(pkg->src);
    pkg->src = 0;

    if(pkg->dependencies)
        list_destroy(pkg->dependencies);
    pkg->dependencies = 0;

    if(pkg->development) 
        list_destroy(pkg->development);
    pkg->development = 0;

    free(pkg);
    pkg = 0;
}


void freeDeps(Dependency *dep) 
{
    free(dep->name);
    free(dep->author);
    free(dep->version);
    free(dep);
}








































































































































































































































































