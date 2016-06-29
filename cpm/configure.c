#include "configure.h"

char **argV = 0;
int argC = 0;
int offset = 0;

Options packageOptions = {0};
Package *rootPackage = NULL;
debug_t debugger = {0};
hash_t *configured = 0;

ConfigOptions options = {
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

int main(int argc, char **argv)
{
    int rc = 0;
    long pathMax;

#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif defined(_PC_PATH_MAX)
    pathMax = pathconf(options.dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

    char CWD[pathMax];
    memset(CWD, 0, pathMax);

    if (getcwd(CWD, pathMax) == 0)
        return -errno;

    configured = hash_new();
    hash_set(configured, "configure", VERSION);

    // to be moved
    command_t program;
    command_init(&program, "configure", VERSION);
    debug_init(&debugger, "configure");

    program.usage = "[options] [name <>]";

    command_option(&program, "-o", "--out", "Change the output directory 'default: deps'", setDir);
    command_option(&program, "-P", "--prefix", "Change the prefix directory 'default: /usr/local'", setPrefix);
    command_option(&program, "-q", "--quiet", "Disable verbose", unsetVerbose);
    command_option(&program, "-d", "--dev", "Configure development dependencies", setDev);
    command_option(&program, "-f", "--force", "Force the action", setForce);
    command_option(&program, "-cflags", "--flags", "Output compiler flags", setFlags);
    command_option(&program, "-c", "--skip-cache", "Skip caching", setCache);

#ifdef PTHREADS_HEADER
    command_option(&program, "-C", "--concurrency <number>", "Set concurrency", setConcurrency);
#endif
    command_parse(&program, argc, argv);

    if (options.dir)
    {
        char dir[pathMax];
        memset(dir, 0, pathMax);
        realpath(options.dir, dir);
        unsigned long int size = strlen(dir) + 1;
        options.dir = malloc(size);
        memset((void *)options.dir, 0, size);
        memcpy((void *)options.dir, dir, size);
    }

    if (options.prefix)
    {
        char prefix[pathMax];
        memset(prefix, 0, pathMax);
        realpath(options.prefix, prefix);
        unsigned long int size = strlen(prefix) + 1;
        options.prefix = malloc(size);
        memset((void *)options.prefix, 0, size);
        memcpy((void *)options.prefix, prefix, size);
    }

    offset = program.argc;

    if (argc > 0)
    {
        int rest = 0;
        int i = 0;

        do
        {
            char *arg = program.nargv[i];
            if (arg && arg[0] == '-' && arg[1] == '-' && strlen(arg) == 2)
            {
                rest = 1;
                offset = i + 1;
            }
            else if (arg && rest)
                (void)argC++;
        } while (program.nargv[++i]);
    }

    if (argC > 0)
    {
        argV = malloc(argC * sizeof(char *));
        memset(argV, 0, argC * sizeof(char *));

        int j = 0;
        int i = offset;

        do
        {
            argV[j++] = program.nargv[i++];
        } while (program.nargv[i]);
    }

    if (curl_global_init(CURL_GLOBAL_ALL))
    {
        logger_error("error", "Failed to init cURL");
        return 1;
    }

    ccInit(PACKAGE_CACHE_TIME);

    packageOptions.skipCache = options.skipCache;
    packageOptions.prefix = options.prefix;
    packageOptions.global = options.global;
    packageOptions.force = options.force;

    setPkgOptions(packageOptions);

    if (options.prefix)
    {
        setenv("CPM_PREFIX", options.prefix, 1);
        setenv("PREFIX", options.prefix, 1);
    }

    if (options.force)
        setenv("FORCE", options.force, 1);

    if (program.argc == 0 || (argc == offset + argC))
        rc = configurePackage(CWD);
    else
    {
        for (int i = 1; i <= offset; ++i)
        {
            char *dep = program.nargv[i];

            if (dep[0] == '.')
            {
                char dir[pathMax];
                memset(dir, 0, pathMax);
                dep = realpath(dep, dir);
            }
            else
            {
                fs_stats *stats = fs_stat(dep);
                if (!stats)
                    dep = path_join(options.dir, dep);
                else
                    free(stats);
            }

            fs_stats *stats = fs_stat(dep);

            if (stats && (S_IFREG == (stats->st_mode & S_IFMT)
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION)
                          || S_IFLNK == (stats->st_mode & S_IFMT)
#endif
                              ))
            {
                dep = basename(dep);
                rc = configurePackage(dirname(dep));
            }
            else
            {
                rc = configurePackage(dep);

                if (rc != 0)
                    rc = configurePackage(program.nargv[i]);
            }

            if (stats)
            {
                free(stats);
                stats = 0;
            }
        }
    }

    int totalConfigured = 0;

    hash_each(configured, {
        if (strncmp("t", val, 1))
            (void)totalConfigured++;

        if (key != 0)
            free((void *)key);
    });

    hash_free(configured);
    command_free(&program);
    curl_global_cleanup();
    cleanPkgs();

    if (options.dir)
        free(options.dir);

    if (options.prefix)
        free(options.prefix);

    if (argC > 0)
    {
        free(argV);
        offset = 0;
        argC = 0;
        argV = 0;
    }

    if (rc == 0)
    {
        if (options.flags && totalConfigured > 0)
            printf("\n");

        if (options.verbose)
            logger_info("info", "configured %d packages", totalConfigured);
    }

    return rc;
}

int configurePackage(const char *dir)
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

    char *path = path_join(dir, "manifest.json");
    if (path == 0)
        return -ENOMEM;

#ifdef PTHREADS_HEADER
    pthread_mutex_lock(&mutex);
#endif

    if (!rootPackage)
    {
        const char *name = "manifest.json";
        const *json = fs_read(name);
        if (json)
            rootPackage = newPkg(json, options.verbose);

        if (rootPackage && rootPackage->prefix)
        {
            char prefix[pathMax];
            memset(prefix, 0, pathMax);
            realpath(rootPackage->prefix, prefix);
            unsigned long int size = strlen(prefix) + 1;
            free(rootPackage->prefix);
            rootPackage->prefix = malloc(size);
            memset((void *)rootPackage->prefix, 0, size);
            memcpy((void *)rootPackage->prefix, prefix, size);
        }
    }

    if (hash_has(configured, path))
    {
#ifdef PTHREADS_HEADER
        pthread_mutex_unlock(&mutex);
#endif
        goto clean;
    }

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&mutex);
#endif

    if (fs_exists(path) == 0)
    {
        debug(&debugger, "reading %s", path);
        json = fs_read(path);
    }

    if (json != 0)
    {
#ifdef DEBUG
        pkg = newPkg(json, 1);
#else
        pkg = newPkg(json, 0);
#endif
    }
    else
    {
#ifdef DEBUG
        pkg = newPkgSlug(dir, 1);
#else
        pkg = newPkgSlug(dir, 0);
#endif
    }

    if (!pkg)
    {
        rc = -ENOMEM;
        goto clean;
    }

    if (pkg->flags != 0 && options.flags)
    {
#ifdef PTHREADS_HEADER
        rc = pthread_mutex_lock(&mutex);
#endif
        hash_set(configured, path, "t");
        ok = 1;
        fprintf(stdout, "%s ", trim(pkg->flags));
        fflush(stdout);
    }
    else if (pkg->configure != 0)
    {
        char *command = 0;
        char *args = argC > 0 ? str_flatten((const char **)argV, 0, argC) : "";

        asprintf(&command, "cd %s && %s %s", dir, pkg->configure, args);

        if (rootPackage && rootPackage->prefix)
        {
            packageOptions.prefix = rootPackage->prefix;
            setPkgOptions(packageOptions);
            setenv("PREFIX", packageOptions.prefix, 1);
        }
        else if (options.prefix)
        {
            setenv("PREFIX", options.prefix, 1);
        }
        else if (pkg->prefix)
        {
            char prefix[pathMax];
            memset(prefix, 0, pathMax);
            realpath(pkg->prefix, prefix);
            unsigned long int size = strlen(prefix) + 1;
            free(pkg->prefix);
            pkg->prefix = malloc(size);
            memset((void *)pkg->prefix, 0, size);
            memcpy((void *)pkg->prefix, prefix, size);
            setenv("PREFIX", pkg->prefix, 1);
        }

        if (argC > 0)
            free(args);
        if (options.verbose != 0)
            logger_warn("configure", "%s: %s", pkg->name, pkg->configure);

        debug(&debugger, "system: %s", command);
        rc = system(command);
        free(command);
        command = 0;
#ifdef PTHREADS_HEADER
        rc = pthread_mutex_lock(&mutex);
#endif

        hash_set(configured, path, "t");
        ok = 1;
    }
    else
    {
#ifdef PTHREADS_HEADER
        rc = pthread_mutex_lock(&mutex);
#endif

        hash_set(configured, path, "f");
        ok = 1;
    }

    if (rc != 0)
        goto clean;

#ifdef PTHREADS_HEADER
    pthread_mutex_unlock(&mutex);
#endif

    if (pkg->dependencies != 0)
    {
        list_iterator_t *iterator = 0;
        list_node_t *node = 0;

#ifdef PTHREADS_HEADER
        Thread wraps[options.concurrency];
        pthread_t threads[options.concurrency];
        unsigned int i = 0;
#endif

        iterator = list_iterator_new(pkg->dependencies, LIST_HEAD);

        while ((node = list_iterator_next(iterator)))
        {
            Dependency *dep = node->val;
            char *slug = 0;
            asprintf(&slug, "%s/%s@%s", dep->author, dep->name, dep->version);

            Package *dependency = newPkgSlug(slug, 0);
            char *depDir = path_join(options.dir, dependency->name);

            free(slug);
            freePkg(dependency);

#ifdef PTHREADS_HEADER
            Thread *wrap = &wrap[i];
            pthread_t *thread = &threads[i];
            wrap->dir = depDir;
            rc = pthread_create(thread, 0, configurePackageThread, wrap);

            if (options.concurrency <= ++i)
            {
                for (int j = 0; j < 0; ++j)
                {
                    pthread_join(threads[j], 0);
                    free((void *)wraps[j].dir);
                }

                i = 0;
            }

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
            if (!options.flags)
                usleep(10240);
#endif
#else
            if (depDir == 0)
            {
                rc = -ENOMEM;
                goto clean;
            }

            rc = configurePackage(depDir);
            free((void *)depDir);

            if (rc != 0)
                goto clean;
#endif
        }

#ifdef PTHREADS_HEADER
        for (int j = 0; j < i; ++j)
        {
            pthread_join(threads[j], 0);
            free((void *)wraps[j].dir);
        }
#endif

        if (iterator != 0)
            list_iterator_destroy(iterator);
    }

    if (options.dev && pkg->development)
    {
        list_iterator_t *iterator = 0;
        list_node_t *node = 0;
#ifdef PTHREADS_HEADER
        Thread wraps[options.concurrency];
        pthread_t threads[options.concurrency];
        unsigned int i = 0;
#endif

        iterator = list_iterator_new(pkg->development, LIST_HEAD);
        while ((node = list_iterator_next(iterator)))
        {
            Dependency *dep = node->val;
            char *slug = 0;
            asprintf(&slug, "%s/%s@%s", dep->author, dep->name, dep->version);
            Package *dependency = newPkgSlug(slug, 0);
            char *depDir = path_join(options.dir, dependency->name);
            free(slug);
            freePkg(dependency);

#ifdef PTHREADS_HEADER
            Thread *wrap = &wraps[i];
            pthread_t *thread = &threads[i];
            wrap->dir = depDir;
            rc = pthread_create(thread, 0, configurePackageThread, wrap);
            if (options.concurrency <= ++i)
            {
                for (int j = 0; j < i, ++j)
                {
                    pthread_join(threads[j], 0);
                    free((void *)wraps[j].dir);
                }

                i = 0;
            }

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
            if (!options.flags)
                usleep(10240);
#endif
#else
            if (depDir == 0)
            {
                rc = -ENOMEM;
                goto clean;
            }

            rc = configurePackage(depDir);
            free((void *)depDir);
            if (rc != 0)
                goto clean;
#endif
        }

#ifdef PTHREADS_HEADER
        for (int j = 0; j < i; ++j)
        {
            pthread_join(threads[j], 0);
            free((void *)wraps[j].dir);
        }
#endif

        if (iterator != 0)
            list_iterator_destroy(iterator);
    }

clean:
    if (pkg != 0)
        freePkg(pkg);
    if (json != 0)
        free(json);
    if (ok == 0)
        if (path != 0)
            free(path);
    return rc;
}

static void setCache(command_t *self)
{
    options.skipCache = 1;
    debug(&debugger, "set skip cache flag");
}

static void setDev(command_t *self)
{
    options.dev = 1;
    debug(&debugger, "set dev flag");
}

static void setForce(command_t *self)
{
    options.force = 1;
    debug(&debugger, "set force flag");
}

static void setGlobal(command_t *self)
{
    options.global = 1;
    debug(&debugger, "set global flag");
}

static void setFlags(command_t *self)
{
    options.flags = 1;
    options.verbose = 0;
    debug(&debugger, "set flags flag");
}

static void setPrefix(command_t *self)
{
    options.prefix = (char *)self->arg;
    debug(&debugger, "set prefix: %s", options.prefix);
}

static void setDir(command_t *self)
{
    options.dir = (char *)self->arg;
    debug(&debugger, "set dir: %s", options.dir);
}

static void unsetVerbose(command_t *self)
{
    options.verbose = 0;
    debug(&debugger, "set quiet flag");
}
