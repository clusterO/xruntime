#include "install.h"

long pathMax;
debug_t debugger = {0};

static InstallOptions options = {0};
static Options packageOptions = {0};
static Package *rootPackage = NULL;

#ifdef PTHREADS_HEADER
static void setConcurrency(command_t *self)
{
    if (self->arg)
    {
        options.concurrency = atol(self->arg);
        debug(&debugger, "set concurrency: %lu", options.concurrency);
    }
}
#endif

int main(int argc, char **argv)
{
    options.verbose = 1;
    options.dev = 0;
#ifdef _WIN32
    options.dir = ".\\deps";
#else
    options.dir = "./deps";
#endif
    if (options.prefix)
    {
#ifdef PATH_MAX
        pathMax = PATH_MAX;
#elif defined(_PC_PATH_MAX)
        pathMax = pathconf(options.dir, _PC_PATH_MAX);
#else
        pathMax = 4096;
#endif
        char prefix[pathMax];
        memset(prefix, 0, pathMax);
        realpath(options.prefix, prefix);
        unsigned long int size = strlen(prefix) + 1;
        options.prefix = malloc(size);
        memset((void *)options.prefix, 0, size);
        memcpy((void *)options.prefix, prefix, size);
    }

    debug_init(&debugger, "install");

    if (createCache(PACKAGE_CACHE_TIME) != 0)
        return -1;

    command_t program;
    getCommandOptions(&program, argc, argv);
    debug(&debugger, "%d arguments", program.argc);

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        logger_error("error", "Failed to initialize cURL");

    packageOptions.skipCache = options.skipCache;
    packageOptions.prefix = options.prefix;
    packageOptions.global = options.global;
    packageOptions.force = options.force;
    packageOptions.token = options.token;
#ifdef PTHREADS_HEADER
    packageOptions.concurrency = options.concurrency;
#endif

    setPackageOptions(packageOptions);

    if (options.prefix)
    {
        setenv("CPM_PREFIX", options.prefix, 1);
        setenv("PREFIX", options.prefix, 1);
    }

    if (options.force)
        setenv("FORCE", "1", 1);

    int code;
    if (program.argc == 0)
        code = installLocalPackages();
    else
        code = installPackages(program.argc, program.argv);

    cleanPackages();
    curl_global_cleanup();
    command_free(&program);

    return code;
}

static int installLocalPackages()
{
    // this file lives in the output folder
    // should be moved to the actual project folder
    const char *file = "manifest.json";

    if (fs_exists(file) == -1)
    {
        logger_error("error", "Missing manifest file");
        printf("Please run 'cpm init' to initialize the project.\n");
        return 1;
    }

    debug(&debugger, "reading local manifest");

    char *json = fs_read(file);
    if (json == NULL)
        return 1;

    Package *pkg = newPackage(json, options.verbose);
    if (pkg == NULL)
        goto e1;

    if (pkg->prefix)
        setenv("PREFIX", pkg->prefix, 1);

    int rc = installDependency(pkg, options.dir, options.verbose);
    if (rc == -1)
        goto e2;

    if (options.dev)
    {
        rc = installDevPackage(pkg, options.dir, options.verbose);
        if (rc == -1)
            goto e2;
    }

    free(json);
    freePackage(pkg);
    return 0;

e2:
    freePackage(pkg);
e1:
    printf("e1");
    free(json);
    return 1;
}

static int installPackages(int n, char **packages)
{
    for (int i = 0; i < n; i++)
    {
        if (installPackage(packages[i]) == -1)
            return 1;

        debug(&debugger, "install %s (%d)", packages[i], i);
    }

    return 0;
}

static int installPackage(const char *slug)
{
    Package *package = NULL;
    int rc;

    if (!rootPackage)
    {
        const char *name = "manifest.json";
        char *json = fs_read(name);

        if (json)
            rootPackage = newPackage(json, options.verbose);
    }

    if (slug[0] == '.')
        if (strlen(slug) == 1 || (slug[1] == '/' && strlen(slug) == 2)) // risk of condition at 1
        {
            char dir[pathMax];
            realpath(slug, dir);
            slug = dir;

            return installLocalPackages("manifest.json");
        }

    if (fs_exists(slug) == 0)
    {
        fs_stats *stats = fs_stat(slug);

        if (stats != NULL && (S_IFREG == (stats->st_mode & S_IFMT)
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION)
                              || S_IFLNK == (stats->st_mode & S_IFMT)
#endif
                                  ))
        {
            free(stats);
            return installLocalPackages(slug);
        }

        if (stats)
            free(stats);
    }

    if (!package)
        package = newPackageSlug(slug, options.verbose);

    if (package == NULL)
        return -1;

    printf("another delimiter\n");

    if (rootPackage && rootPackage->prefix)
    {
        packageOptions.prefix = rootPackage->prefix;
        setPackageOptions(packageOptions);
    }

    printf("root package prefix checked\n");

    rc = installRootPackage(package, options.dir, options.verbose);
    if (rc != 0)
        goto clean;

    if (rc == 0 && options.dev)
    {
        rc = installDevPackage(package, options.dir, options.verbose);
        if (rc != 0)
            goto clean;
    }

    if (package->repo == 0 || strcmp(slug, package->repo) != 0)
        package->repo = strdup(slug);

    if (options.save)
        saveDependency(package);
    if (options.savedev)
        saveDevDependency(package);

clean:
    freePackage(package);
    return rc;
}

static void getCommandOptions(command_t *program, int argc, char **argv)
{
    command_init(program, "install", VERSION);
    program->usage = "[options] [name <>]";

    command_option(program, "-o", "--out <dir>", "Change the output directory 'default: deps'", setDir);
    command_option(program, "-P", "--prefix <dir>", "Change the prefix directory 'default: /usr/local'", setPrefix);
    command_option(program, "-q", "--quiet", "Disable verbose", unsetVerbose);
    command_option(program, "-d", "--dev", "install develmopent dependency", setDev);
    command_option(program, "-S", "--save", "Save dependency in manifest.json", setSave);
    command_option(program, "-D", "--save-dev", "Save develmopent dependency to manifest.json", setSaveDev);
    command_option(program, "-f", "--force", "Force the action", setForce);
    command_option(program, "-c", "--skip-cache", "Skip cache when installing", setSkipCache);
    command_option(program, "-g", "--global", "Global install", setGlobal);
    command_option(program, "-t", "--token <token>", "Set access token", setToken);
#ifdef PTHREADS_HEADER
    command_option(&program, "-C", "--concurrency <number>", "Set concurrency", setConcurrency);
#endif
    command_parse(program, argc, argv);
}

static int writeDependency(Package *pkg, char *prefix)
{
    const char *file = "manifest.json";
    JSON_Value *pkgJson = json_parse_file(file);
    JSON_Object *pkgJsonObj = json_object(pkgJson);
    JSON_Value *newDep = NULL;

    if (pkgJson == NULL || pkgJsonObj == NULL)
        return 1;

    JSON_Object *dep = json_object_dotget_object(pkgJsonObj, prefix);
    if (dep == NULL)
    {
        newDep = json_value_init_object();
        dep = json_value_get_object(newDep);
        json_object_set_value(pkgJsonObj, prefix, newDep);
    }

    json_object_set_string(dep, pkg->repo, pkg->version);

    int rc = json_serialize_to_file_pretty(pkgJson, file);
    json_value_free(pkgJson);
    return rc;
}

static int saveDependency(Package *pkg)
{
    debug(&debugger, "savind dependency %s at %s", pkg->name, pkg->version);
    return writeDependency(pkg, "dependencies");
}

static int saveDevDependency(Package *pkg)
{
    debug(&debugger, "savind dev dependency %s at %s", pkg->name, pkg->version);
    return writeDependency(pkg, "develmopent");
}

static void setDir(command_t *self)
{
    options.dir = (char *)self->arg;
    debug(&debugger, "set dir: %s", options.dir);
}

static void setPrefix(command_t *self)
{
    options.prefix = (char *)self->arg;
    debug(&debugger, "set prefix: %s", options.prefix);
}

static void setToken(command_t *self)
{
    options.token = (char *)self->arg;
    debug(&debugger, "set token: %s", options.token);
}

static void unsetVerbose(command_t *self)
{
    options.verbose = 0;
    debug(&debugger, "unset verbose");
}

static void setDev(command_t *self)
{
    options.dev = 1;
    debug(&debugger, "set develmopent flag");
}

static void setSave(command_t *self)
{
    options.save = 1;
    debug(&debugger, "set save flag");
}

static void setSaveDev(command_t *self)
{
    options.savedev = 1;
    debug(&debugger, "set save develmopent flag");
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

static void setSkipCache(command_t *self)
{
    options.skipCache = 1;
    debug(&debugger, "set skip cache flag");
}
