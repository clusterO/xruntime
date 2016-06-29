#include "package.h"

CURLSH *cpcs;
debug_t _debugger;

static Lock lock = {PTHREAD_MUTEX_INITIALIZER};
static hash_t *visitedPackages = 0;
static Options defaultOptions = {
    .skipCache = 1,
    .prefix = 0,
    .global = 0,
    .force = 0,
    .token = 0,
#ifdef PTHREADS_HEADER
    .concurrency = 4,
#endif
};

Package *loadPackage(const char *manifest, int verbose)
{
    Package *pkg = NULL;

    if (fs_exists(manifest) == -1)
    {
        logger_error("error", "Missin %s", manifest);
        return NULL;
    }

    logger_info("info", "reading local %s", manifest);

    char *json = fs_read(manifest);
    if (json == NULL)
        goto e1;
    pkg = newPackage(json, verbose);

e1:
    free(json);

    return pkg;
}

Package *loadManifest(int verbose)
{
    Package *pkg = NULL;
    const char *name = "package.json";
    pkg = loadPackage(name, verbose);

    return pkg;
}

Package *newPackage(const char *json, int verbose)
{
    Package *package = NULL;
    JSON_Value *root = NULL;
    JSON_Object *jsonObj = NULL;
    JSON_Array *src = NULL;
    JSON_Object *deps = NULL;
    JSON_Object *devs = NULL;
    int error = 1;

    if (!json)
    {
        if (verbose)
            logger_error("error", "Missing JSON");

        goto clean;
    }

    if (!(root = json_parse_string(json)))
    {
        if (verbose)
            logger_error("error", "Unable to parse JSON");

        goto clean;
    }

    if (!(jsonObj = json_value_get_object(root)))
    {
        if (verbose)
            logger_error("error", "Invalid file");

        goto clean;
    }

    if (!(package = malloc(sizeof(Package))))
        goto clean;

    memset(package, 0, sizeof(Package));

    package->json = strdup(json);
    package->name = jsonObjectGetStringSafe(jsonObj, "name");
    package->repo = jsonObjectGetStringSafe(jsonObj, "repo");
    package->version = jsonObjectGetStringSafe(jsonObj, "version");
    package->license = jsonObjectGetStringSafe(jsonObj, "license");
    package->description = jsonObjectGetStringSafe(jsonObj, "description");
    package->configure = jsonObjectGetStringSafe(jsonObj, "configure");
    package->install = jsonObjectGetStringSafe(jsonObj, "install");
    package->makefile = jsonObjectGetStringSafe(jsonObj, "makefile");
    package->prefix = jsonObjectGetStringSafe(jsonObj, "prefix");
    package->flags = jsonObjectGetStringSafe(jsonObj, "flags");

    if (!package->flags)
        package->flags = jsonObjectGetStringSafe(jsonObj, "cflags");
    if (!package->flags)
    {
        JSON_Array *flags = json_object_get_array(jsonObj, "flags");

        if (!flags)
            flags = json_object_get_array(jsonObj, "cflags");

        if (flags)
        {
            for (int i = 0; i < json_array_get_count(flags); i++)
            {
                char *flag = jsonArrayGetStringSafe(flags, i);

                if (flag)
                {
                    if (!package->flags)
                        package->flags = "";

                    if (asprintf(&package->flags, "%s %s", package->flags, flag) == -1)
                        goto clean;

                    free(flag);
                }
            }
        }
    }

    if (!package->repo && package->author && package->name)
    {
        asprintf(&package->repo, "%s/%s", package->author, package->name);
        _debug("Creating package: %s", package->repo);
    }

    if (!package->author)
        _debug("Unable to determine package author for: %s", package->name);

    if (package->repo)
    {
        // need some edit to get the correct values
        package->author = GetRepoOwner(package->repo, OWNER);
        package->reponame = GetRepoName(package->repo);
    }
    else
    {
        if (verbose)
            logger_warn("warning", "Missing repo for %s", package->name);

        package->author = NULL;
        package->reponame = NULL;
    }

    src = json_object_get_array(jsonObj, "src");

    if (!src)
        src = json_object_get_array(jsonObj, "files");

    if (src)
    {
        if (!(package->src = list_new()))
            goto clean;

        package->src->free = free;

        for (int i = 0; i < json_array_get_count(src); i++)
        {
            char *file = jsonArrayGetStringSafe(src, i);
            _debug("file: %s", file);

            if (!file)
                goto clean;

            if (!(list_rpush(package->src, list_node_new(file))))
                goto clean;
        }
    }
    else
    {
        _debug("No source files listed");
        package->src = NULL;
    }

    if ((deps = json_object_get_object(jsonObj, "dependencies")))
    {
        if (!(package->dependencies = parseDependencies(deps)))
            goto clean;
    }
    else
    {
        _debug("No dependencies listed");
        package->dependencies = NULL;
    }

    if ((devs = json_object_get_object(jsonObj, "development")))
    {
        if (!(package->development = parseDependencies(devs)))
            goto clean;
    }
    else
    {
        _debug("No development dependencies listed");
        package->development = NULL;
    }

    error = 0;

clean:
    if (root)
        json_value_free(root);
    if (error && package)
    {
        freePackage(package);
        package = NULL;
    }

    return package;
}

Package *newPackageSlug(const char *slug, int verbose)
{
    Package *package = NULL;
    const char *name = "package.json";
    unsigned int i = 0;

    package = packageSlug(slug, verbose, name);
    if (package != NULL)
        package->filename = (char *)name;

    return package;
}

Package *packageSlug(const char *slug, int verbose, const char *file)
{
    http_get_response_t *res = NULL;
    Package *package = NULL;
    char *author = NULL;
    char *name = NULL;
    char *version = NULL;
    char *url = NULL;
    char *jsonUrl = NULL;
    char *repo = NULL;
    char *json = NULL;
    char *log = NULL;
    int retries = 3;

    if (!slug)
        goto error;

    _debug("Creating package %s", slug);

    if (!(author = GetRepoOwner(slug, OWNER)))
        goto error;
    if (!(name = GetRepoName(slug)))
        goto error;
    if (!(version = GetRepoVersion(slug, VERSION))) // return master
        goto error;
    if (!(url = packageUrl(author, name, version)))
        goto error;
    if (!(jsonUrl = buildFileUrl(url, file)))
        goto error;

    _debug("author: %s\nname: %s\nversion: %s\n", author, name, version);

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

    if (cacheConfigExists(author, name, version))
    {
        if (defaultOptions.skipCache)
        {
            deleteCacheConfig(author, name, version);
            download(jsonUrl);
        }
        else
        {
            json = getCacheConfig(author, name, version);
            if (!json)
                download(jsonUrl);
            else
                log = "cache";
        }

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }
    else
    {
        // json_url: https://raw.githubusercontent.com/cpm/cassandra:master/package.json
        while (retries-- > 0 && (!res || res->ok))
            res = download(jsonUrl);

        if (retries <= 0)
            goto error;

        log = "fetch";
    }

    if (verbose)
        logger_info(log, "%s/%s:%s", author, name, version);

    free(jsonUrl);
    jsonUrl = NULL;
    free(name);
    name = NULL;

    if (json)
        package = newPackage(json, verbose);
    if (!package)
        goto error;

    if (package->version)
    {
        if (version)
        {
            if (strcmp(version, VERSION) != 0)
            {
                _debug("Version number: %s (%s)", version, package->version);
                free(package->version);
                package->version = version;
            }
            else
                free(version);
        }
    }
    else
        package->version = version;

    if (author && package->author)
    {
        if (strcmp(author, package->author) != 0)
        {
            free(package->author);
            package->author = author;
        }
        else
            free(author);
    }
    else
        package->author = strdup(author);

    if (!(repo = buildRepo(package->author, package->name)))
        goto error;

    if (package->repo)
    {
        if (strcmp(repo, package->repo) != 0)
        {
            free(url);
            if (!(url = packageRepoUrl(package->repo, package->version)))
                goto error;
        }

        free(repo);
        repo = NULL;
    }
    else
        package->repo = repo;

    package->url = url;

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif

    if (package && package->author && package->name && package->version)
    {
        if (setCacheConfig(package->author, package->name, package->version, json) == -1)
            _debug("Failed to cache: %s/%s:%s", package->author, package->name, package->version);
        else
            _debug("Cached: %s/%s:%s", package->author, package->name, package->version);
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

    if (res)
    {
        http_get_free(res);
        json = NULL;
        res = NULL;
    }
    else
    {
        free(json);
        json = NULL;
    }

    return package;

error:
    if (retries == 0)
        if (verbose && author && name && file)
            logger_warn("warning", "Unable to fetch %s/%s:%s", author, name, file);

    free(author);
    free(name);
    free(version);
    free(url);
    free(jsonUrl);
    free(repo);

    if (!res && json)
        free(json);
    if (res)
        http_get_free(res);
    if (package)
        freePackage(package);

    return NULL;
}

Dependency *newDependency(const char *repo, const char *version)
{
    if (!repo || !version)
        return NULL;

    Dependency *dep = malloc(sizeof(Dependency));
    if (!dep)
        return NULL;

    dep->version = strcmp("*", version) == 0 ? strdup(VERSION) : strdup(version);
    dep->name = packageName(repo);
    dep->author = packageAuthor(repo);

    _debug("dependency: %s/%s:%s", dep->author, dep->name, dep->version);
    return dep;
}

int installExecutable(Package *package, char *dir, int verbose)
{
    int rc;
    char *url = NULL;
    char *file = NULL;
    char *tarball = NULL;
    char *command = NULL;
    char *unpackDir = NULL;
    char *deps = NULL;
    char *tmp = NULL;
    char *reponame = NULL;
    long pathMax;
#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif define(_PC_PATH_MAX)
    pathMax = pathconf(dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif
    char dirPath[pathMax];

    _debug("Install executable %s", package->repo);

    tmp = gettempdir();

    if (tmp == NULL)
    {
        if (verbose)
            logger_error("error", "gettempdir() out of memory");
        return -1;
    }

    if (!package->repo)
    {
        if (verbose)
            logger_error("error", "Repo field missing");
        return -1;
    }

    reponame = strrchr(package->repo, '/');
    if (reponame && *reponame != '\0')
        reponame++;
    else
    {
        if (verbose)
            logger_error("error", "Repo name should formatted as user/pkg");
        return -1;
    }

    E_FORMAT(&url, "https://github.com/%s/archive/%s.tar.gz", package->repo, package->version);
    E_FORMAT(&file, "%s-%s.tar.gz", reponame, package->version);
    E_FORMAT(&tarball, "%s/%s", tmp, file);

    rc = http_get_file_shared(url, tarball, cpcs);
    if (rc != 0)
    {
        if (verbose)
            logger_error("error", "Failed to download '%s@%s' - HTTP GET '%s'", package->repo, package->version, url);
        goto clean;
    }

    E_FORMAT(&command, "cd %s && gzip - dc %s | tar x", tmp, file);

    _debug("download url: %s", url);
    _debug("file: %s", file);
    _debug("tarball: %s", tarball);
    _debug("command(extract): %s", command);

    rc = system(command);
    if (rc != 0)
        goto clean;

    free(command);
    command = NULL;

    if (defaultOptions.prefix != NULL || package->prefix != NULL)
    {
        char path[pathMax];
        memset(path, 0, pathMax);

        if (defaultOptions.prefix)
            realpath(defaultOptions.prefix, path);
        else
            realpath(package->prefix, path);

        _debug("env PREFIX: %s", path);
        setenv("PREFIX", path, 1);
        mkdirp(path, 0777);
    }

    const char *configure = package->configure;
    if (configure == 0)
        configure = ":";

    memset(dirPath, 0, pathMax);
    realpath(dir, dirPath);

    char *version = package->version;
    if (version[0] == 'v')
        version++;

    E_FORMAT(&unpackDir, "%s/%s-%s", tmp, reponame, version);
    _debug("dir: %s", unpackDir);

    if (package->dependencies)
    {
        E_FORMAT(&deps, "%s/deps", unpackDir);
        _debug("deps: %s", deps);
        rc = installDependency(package, deps, verbose);
        if (rc == -1)
            goto clean;
    }

    if (!defaultOptions.global && package->makefile)
    {
        E_FORMAT(&command, "cp -fr %s/%s/%s %s", dirPath, package->name, basename(package->makefile), unpackDir);

        rc = system(command);
        if (rc != 0)
            goto clean;
        free(command);
    }

    if (package->flags)
    {
        char *flags = NULL;
#ifdef _GNU_SOURCE
        char *cflags = secure_getenv("CFLAGS");
#else
        char *cflags = getenv("CFLAGS");
#endif

        if (cflags)
            asprintf(&flags, "%s %s", cflags, package->flags);
        else
            asprintf(&flags, "%s", package->flags);

        setenv("CFLAGS", cflags, 1);
    }

    E_FORMAT(&command, "cd %s && %s", unpackDir, package->install);
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

int installRootPackage(Package *package, const char *dir, int verbose)
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
    int max = defaultOptions.concurrency;
#endif

#ifdef PACKAGE_PREFIX
    if (defaultOptions.prefix == 0)
    {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
        defaultOptions.prefix = PACKAGE_PREFIX;
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }
#endif

    if (defaultOptions.prefix == 0)
    {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
#ifdef _GNU_SOURCE
        char *prefix = secure_getenv("PREFIX");
#else
        char *prefix = getenv("PREFIX");
#endif

        if (prefix)
            defaultOptions.prefix = prefix;

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if (visitedPackages == 0)
    {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
        visitedPackages = hash_new();
        hash_set(visitedPackages, strdup(""), "");

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if (defaultOptions.force == 0 && package && package->name)
    {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif

        if (hash_has(visitedPackages, package->name))
        {
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
    Thread **fetchs = 0;
    if (pkg != NULL && pkg->src != NULL)
        if (pkg->src->len > 0)
            fetchs = malloc(pkg->src->len * sizeof(Thread));

    if (fetchs)
        memset(fetchs, 0, pkg->src->len * sizeof(Thread));
#endif

    if (!package || !dir)
    {
        rc = -1;
        goto clean;
    }

    if (!(pkgDir = path_join(dir, package->name)))
    {
        rc = -1;
        goto clean;
    }

    if (!defaultOptions.global)
    {
        _debug("mkdir -p %s", pkgDir);
        if (mkdirp(pkgDir, 0777) == -1)
        {
            rc = -1;
            goto clean;
        }
    }

    if (package->url == NULL)
    {
        package->url = packageUrl(package->author, package->reponame, package->version);
        if (package->url == NULL)
        {
            rc = -1;
            goto clean;
        }
    }

    if (!(pkgJson = path_join(pkgDir, package->filename)))
    {
        rc = -1;
        goto clean;
    }

    if (!defaultOptions.global && package->src != NULL)
    {
        _debug("write: %s", pkgJson);
        if (fs_write(pkgJson, package->json) == -1)
        {
            if (verbose)
                logger_error("error", "Failed to write %s", pkgJson);
            rc = -1;
            goto clean;
        }
    }

    if (package->name)
    {
#ifdef PTHREADS_HEADER
        pthread_mutex_lock(&lock.mutex);
#endif
        if (!hash_has(visitedPackages, package->name))
            hash_set(visitedPackages, strdup(package->name), "t");
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if (!defaultOptions.global && package->makefile)
    {
        _debug("fetch: %s/%s", package->repo, package->makefile);
        void *fetch = 0;
        rc = fetchPackage(package, pkgDir, package->makefile, verbose, &fetch);
        if (rc != 0)
            goto clean;

#ifdef PTHREADS_HEADER
        if (fetch != 0)
        {
            Thread *data = fetch;
            int *status;
            pthread_join(data->thread, (void **)&status);
            if (status != NULL)
            {
                rc = *status;
                free(status);
                status = 0;
                if (rc != 0)
                {
                    rc = 0;
                    logger_warn("warning", "Unable to fetch Makefile (%s) fir '%s'", pkg->makefile, pkg->name);
                }
            }
        }
#endif
    }

    if (defaultOptions.global || package->src == NULL)
        goto install;

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif

    if (cacheConfigExists(package->author, package->name, package->version))
    {
        if (defaultOptions.skipCache)
        {
            deleteCacheConfig(package->author, package->name, package->version);
#ifdef PTHREADS_HEADER
            pthread_mutex_unlock(&lock.mutex);
#endif
            goto download;
        }

        if (verbose)
            logger_info("Cache", package->repo);

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
        goto install;
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

download:
    iterator = list_iterator_new(package->src, LIST_HEAD);
    list_node_t *source;

    while ((source = list_iterator_next(iterator)))
    {
        void *fetch = NULL;
        rc = fetchPackage(package, pkgDir, source->val, verbose, &fetch);
        if (rc != 0)
        {
            list_iterator_destroy(iterator);
            iterator = NULL;
            rc = -1;
            goto clean;
        }

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
        usleep(1024 * 10);
#endif
#ifdef PTHREADS_HEADER
        if (i < 0)
            i = 0;
        fetchs[i] = fetch;
        pending++;

        if (i < max)
            i++;
        else
        {
            while (--i >= 0)
            {
                Thread *data = fetch[i];
                int *status;
                pthread_join(data->thread, (void **)&status);
                free(data);
                fetchs[i] = NULL;

                pending--;

                if (NULL != status)
                {
                    rc = *status;
                    free(status);
                    status = 0;
                }

                if (rc != 0)
                {
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
    while (--i >= 0)
    {
        Thread *data = fetch[i];
        int *status;
        pthread_join(data->thread, (void **)&status);
        pending--;
        free(data);
        fetchs[i] = NULL;

        if (status == NULL)
        {
            rc = *status;
            free(status);
            status = 0;
        }

        if (rc != 0)
        {
            rc = -1;
            goto clean;
        }
    }
#endif
#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif

    setCacheConfig(package->author, package->name, package->version, pkgDir);

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

install:
    if (package->configure)
    {
        if (defaultOptions.prefix != NULL && package->prefix != NULL)
        {
            char path[pathMax];
            memset(path, 0, pathMax);

            if (defaultOptions.prefix)
                realpath(defaultOptions.prefix, path);
            else
                realpath(package->prefix, path);

            _debug("env PREFIX: %s", path);
            setenv("PREFIX", path, 1);
        }

        E_FORMAT(&command, "cd %s/%s && %s", dir, package->name, package->configure);
        _debug("command(configure): %s", command);
        rc = system(command);
        if (rc != 0)
            goto clean;
    }

    if (rc == 0 && package->install)
        rc = installExecutable(package, dir, verbose);
    if (rc == 0)
        rc = installDependency(package, dir, verbose);

clean:
    if (pkgDir)
        free(pkgDir);
    if (pkgJson)
        free(pkgJson);
    if (iterator)
        list_iterator_destroy(iterator);
    if (command)
        free(command);
#ifdef PTHREADS_HEADER
    if (pkg != NULL && pkg->src != NULL)
        if (pkg->src->len > 0)
            if (fetchs)
                free(fetchs);
    fetchs = NULL;
#endif
    return rc;
}

int installDependency(Package *package, const char *dir, int verbose)
{
    if (!package || !dir)
        return -1;

    if (package->dependencies == NULL)
        return 0;

    return install(package->dependencies, dir, verbose);
}

int installDevPackage(Package *pkg, const char *dir, int verbose)
{
    if (!pkg || !dir)
        return -1;

    if (pkg->development == NULL)
        return 0;

    return install(pkg->development, dir, verbose);
}

void freePackage(Package *pkg)
{
    if (pkg == NULL)
        return;
    if (pkg->refs != 0)
        return;

#define FREE(k)       \
    if (pkg->k)       \
    {                 \
        free(pkg->k); \
        pkg->k = 0;   \
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
    FREE(reponame);
    FREE(url);
    FREE(version);
    FREE(flags);
#undef FREE

    if (pkg->src)
        list_destroy(pkg->src);
    pkg->src = 0;

    if (pkg->dependencies)
        list_destroy(pkg->dependencies);
    pkg->dependencies = 0;

    if (pkg->development)
        list_destroy(pkg->development);
    pkg->development = 0;

    free(pkg);
    pkg = 0;
}

static int fetchFile(Package *pkg, const char *dir, char *file, int verbose)
{
    char *url = NULL;
    char *path = NULL;
    int saved = 0;
    int rc = 0;

    _debug("Fetch file: %s/%s", pkg->repo, file);

    if (pkg == NULL)
        return 1;
    if (pkg->url == NULL)
        return 1;

    if (strncmp(file, "http", 4))
        url = strdup(file);
    else if (!(url = buildFileUrl(pkg->url, file)))
        return 1;

    _debug("File URL: %s", url);

    if (!(path = path_join, (dir, basename(file))))
    {
        rc = 1;
        goto clean;
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&lock.mutex);
#endif

    if (defaultOptions.force == 1 || fs_exists(path) == -1)
    {
        if (verbose)
        {
            logger_info("Fetch", "%s:%s", pkg->repo, file);
            fflush(stdout);
        }

#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
        rc = http_get_file_shared(url, path, cpcs);
        saved = 1;
    }
    else
    {
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&lock.mutex);
#endif
    }

    if (rc == -1)
    {
        if (verbose)
        {
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

    if (saved)
    {
        if (verbose)
        {
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

static int fetchPackage(Package *pkg, const char *dir, char *file, int verbose, void **data)
{
#ifdef PTHREADS_HEADER
    return fetchFile(pkg, dir, file, verbose);
#else
    Thread *fetch = malloc(sizeof(*fetch));
    int rc = 0;

    if (fetch == 0)
        return -1;

    *data = 0;

    memset(fetch, 0, sizeof(*fetch));

    fetch->pkg = pkg;
    fetch->dir = dir;
    fetch->file = file;
    fetch->verbose = verbose;

    rc = pthread_attr_init(&fetch->attr);

    if (rc != 0)
    {
        free(fetch);
        return rc;
    }

    (void)pkg->refs++;
    rc = pthread_create(&fetch->thread, NULL, fetchThreadFile, fetch);

    if (rc != 0)
    {
        pthread_attr_destroy(&fetch->attr);
        free(fetch);
        return rc;
    }

    rc = pthread_attr_destroy(&fetch->attr);

    if (rc != 0)
    {
        pthread_cancel(fetch->thread);
        free(fetch);
        return rc;
    }

    *data = fetch;

    return rc;
#endif
}

void freeDependency(Dependency *dep)
{
    free(dep->name);
    free(dep->author);
    free(dep->version);
    free(dep);
}

void setPackageOptions(Options opts)
{
    if (defaultOptions.skipCache == 1 && opts.skipCache == 0)
        defaultOptions.skipCache = 0;
    else if (defaultOptions.skipCache == 0 && opts.skipCache == 1)
        defaultOptions.skipCache = 1;

    if (defaultOptions.global == 1 && opts.global == 0)
        defaultOptions.global = 0;
    else if (defaultOptions.global == 0 && opts.global == 1)
        defaultOptions.global = 1;

    if (defaultOptions.force == 1 && opts.force == 0)
        defaultOptions.force = 0;
    else if (defaultOptions.force == 0 && opts.force == 1)
        defaultOptions.force = 1;

    if (opts.prefix != 0)
        if (strlen(opts.prefix) == 0)
            defaultOptions.prefix = 0;
        else
            defaultOptions.prefix = 1;

    if (opts.token != 0)
        if (strlen(opts.token) == 0)
            defaultOptions.token = 0;
        else
            defaultOptions.token = 1;

    if (opts.concurrency)
        defaultOptions.concurrency = opts.concurrency;
    else if (opts.concurrency < 0)
        defaultOptions.concurrency = 0;

    if (defaultOptions.concurrency < 0)
        defaultOptions.concurrency = 0;
}

char *packageUrl(const char *author, const char *name, const char *version)
{
    if (!author || !name || !version)
        return NULL;

    int size = strlen(GITHUB_RAW_CONTENT_URL) + strlen(author) + strlen(name) + strlen(version) + 3;

    if (defaultOptions.token != 0)
        size += (strlen(defaultOptions.token) + 1);

    char *slug = malloc(size);
    if (slug)
    {
        memset(slug, '\0', size);
        if (defaultOptions.token != 0)
            sprintf(slug, GITHUB_RAW_CONTENT_AUTH_URL "%s/%s:%s", defaultOptions.token, author, name, version);
        else
            sprintf(slug, GITHUB_RAW_CONTENT_URL "%s/%s:%s", author, name, version);
    }

    return slug;
}

char *packageRepoUrl(const char *repo, const char *version)
{
    if (!repo || !version)
        return NULL;
    int size = strlen(GITHUB_RAW_CONTENT_URL) + strlen(repo) + strlen(version) + 2;

    if (defaultOptions.token != 0)
        size += (strlen(defaultOptions.token) + 1);

    char *slug = malloc(size);
    if (slug)
    {
        memset(slug, '\0', size);
        if (defaultOptions.token != 0)
            sprintf(slug, GITHUB_RAW_CONTENT_AUTH_URL "%s/%s", defaultOptions.token, repo, version);
        else
            sprintf(slug, GITHUB_RAW_CONTENT_URL, "%s/%s", repo, version);
    }

    return slug;
}

char *packageAuthor(const char *slug)
{
    return GetRepoOwner(slug, OWNER);
}

char *packageVersion(const char *slug)
{
    return GetRepoVersion(slug, VERSION);
}

char *packageName(const char *slug)
{
    return GetRepoName(slug);
}

void cleanPackages()
{
    if (visitedPackages != 0)
    {
        hash_each(visitedPackages, {
            free(key);
            val;
        });

        hash_free(visitedPackages);
        visitedPackages = 0;
    }

    curl_share_cleanup(cpcs);
}

static void *fetchThreadFile(void *arg)
{
    Thread *data = arg;
    int *status = malloc(sizeof(int));
    int rc = fetchFile(data->pkg, data->dir, data->file, data->verbose);
    *status = rc;
    (void)data->pkg->refs--;
    pthread_exit((void *)status);
    return (void *)rc;
}

static inline char *jsonObjectGetStringSafe(JSON_Object *obj, const char *key)
{
    const char *val = json_object_get_string(obj, key);
    if (!val)
        return NULL;
    return strdup(val);
}

static inline char *jsonArrayGetStringSafe(JSON_Array *arr, const char *i)
{
    const char *val = json_array_get_string(arr, i);
    if (!val)
        return NULL;
    return strdup(val);
}

static inline char *buildFileUrl(const char *url, const char *file)
{
    if (!url || !file)
        return NULL;

    int size = strlen(url) + strlen(file) + 2;
    char *res = malloc(size);

    if (res)
    {
        memset(res, 0, size);
        sprintf(res, "%s/%s", url, file);
    }

    return res;
}

static inline char *buildSlug(const char *author, const char *name, const char *version)
{
    int size = strlen(author) + strlen(name) + strlen(version) + 3;
    char *slug = malloc(size);

    if (slug)
    {
        memset(slug, '\0', size);
        sprintf(slug, "%s/%s@%s", author, name, version);
    }

    return slug;
}

static inline char *buildRepo(const char *author, const char *name)
{
    int size = strlen(author) + strlen(name) + 2;
    char *repo = malloc(size);

    if (repo)
    {
        memset(repo, '\0', size);
        sprintf(repo, "%s/%s", author, name);
    }

    return repo;
}

static inline int install(list_t *list, const char *dir, int verbose)
{
    list_node_t *node = NULL;
    list_iterator_t *iterator = NULL;
    int rc = -1;

    if (!list || !dir)
        goto clean;

    iterator = list_iterator_new(list, LIST_HEAD);
    if (iterator == NULL)
        goto clean;

    list_t *freeList = list_new();

    while ((node = list_iterator_next(iterator)))
    {
        Dependency *dep = NULL;
        char *slug = NULL;
        Package *pkg = NULL;
        int error = 1;

        dep = (Dependency *)node->val;
        slug = buildSlug(dep->author, dep->name, dep->version);
        if (slug == NULL)
            goto cleanLoop;

        pkg = newPackageSlug(slug, verbose);
        if (pkg == NULL)
            goto cleanLoop;

        if (installRootPackage(pkg, dir, verbose) == -1)
            goto cleanLoop;

        list_rpush(freeList, list_node_new(pkg));
        error = 0;

    cleanLoop:
        if (slug)
            free(slug);
        if (error)
        {
            list_iterator_destroy(iterator);
            iterator = NULL;
            rc = -1;
            goto clean;
        }
    }

    rc = 0;

clean:
    if (iterator)
        list_iterator_destroy(iterator);

    iterator = list_iterator_new(freeList, LIST_HEAD);

    while ((node = list_iterator_new(iterator, LIST_HEAD)))
    {
        Package *pkg = node->val;
        if (pkg)
            freePackage(pkg);
    }

    list_iterator_destroy(iterator);
    list_destroy(freeList);

    return rc;
}

static inline list_t *parseDependencies(JSON_Object *json)
{
    list_t *list = NULL;

    if (!json)
        goto done;
    if (!(list = list_new()))
        goto done;

    list->free = freeDependency;

    for (int i = 0; i < json_object_get_count(json); i++)
    {
        const char *name = NULL;
        char *version = NULL;
        Dependency *dep = NULL;
        int error = 1;

        if (!(name = json_object_get_name(json, i)))
            goto cleanLoop;
        if (!(version = jsonObjectGetStringSafe(json, name)))
            goto cleanLoop;
        if (!(dep = newDependency(name, version)))
            goto cleanLoop;
        if (!(list_rpush(list, list_node_new(dep))))
            goto cleanLoop;

        error = 0;

    cleanLoop:
        if (version)
            free(version);
        if (error)
        {
            list_destroy(list);
            list = NULL;
        }
    }

done:
    return list;
}

static http_get_response_t *download(char *jsonUrl)
{
    http_get_response_t *res;
    char *json;

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&lock.mutex);
#endif

#ifdef PTHREADS_HEADER
    initCurlShare();
    _debug("GET %s", jsonUrl);
    http_get_free(res);
    res = http_get_shared(jsonUrl, cpcs);
#else
    res = http_get(jsonUrl);
#endif

    // processing invalid requests
    json = res->data;
    _debug("status: %d", res->status);

    return res;
}
