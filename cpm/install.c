#include "install.h"

static struct options opts = {0};
static Options pkgOpts = {0};
static Package *rootPkg = NULL;

#ifdef PTHREADS_HEADER
static void setConcurrency(command_t *self)
{
    if (self->arg)
    {
        opts.concurrency = atol(self->arg);
        debug(&debugger, "set concurrency: %lu", opts.concurrency);
    }
}
#endif

int main(int argc, char **argv)
{
    long pathMax;
#ifdef _WIN32
    opts.dir = ".\\deps";
#else
    opts.dir = "./deps";
#endif

    opts.verbose = 1;
    opts.dev = 0;

#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif defined(_PC_PATH_MAX)
    pathMax = pathconf(opts.dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

    debug_init(&debugger, "install");
    ccInit(PACKAGE_CACHE_TIME);

    command_t program;
    command_init(&program, "intall", VERSION);
    program.usage = "[options] [name <>]";

    command_option(&program, "-o", "--out <dir>", "Change the output directory 'default: deps'", setDir);
    command_option(&program, "-P", "--prefix <dir>", "Change the prefix directory 'default: /usr/local'", setPrefix);
    command_option(&program, "-q", "--quiet", "Disable verbose", unsetVerbose);
    command_option(&program, "-d", "--dev", "install develmopent dependency", setDev);
    command_option(&program, "-S", "--save", "Save dependency in manifest.json", setSave);
    command_option(&program, "-D", "--save-dev", "Save develmopent dependency to manifest.json", setSaveDev);
    command_option(&program, "-f", "--force", "Force the action", setForce);
    command_option(&program, "-c", "--skip-cache", "Skip cache when installing", setSkipCache);
    command_option(&program, "-g", "--global", "Global install", setGlobal);
    command_option(&program, "-t", "--token <token>", "Set access token", setToken);
#ifdef PTHREADS_HEADER
    command_option(&program, "-C", "--concurrency <number>", "Set concurrency", setConcurrency);
#endif
    command_parse(&program, argc, argv);

    debug(&debugger, "%d arguments", program.argc);

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        logger_error("error", "Failed to initialize cURL");

    if (opts.prefix)
    {
        char prefix[pathMax];
        memset(prefix, 0, pathMax);
        realpath(opts.prefix, prefix);
        unsigned long int size = strlen(prefix) + 1;
        opts.prefix = malloc(size);
        memset((void *)opts.prefix, 0, size);
        memcpy((void *)opts.prefix, prefix, size);
    }

    ccInit(PACKAGE_CACHE_TIME);

    pkgOpts.skipCache = opts.skipCache;
    pkgOpts.prefix = opts.prefix;
    pkgOpts.global = opts.global;
    pkgOpts.force = opts.force;
    pkgOpts.token = opts.token;

#ifdef PTHREADS_HEADER
    pkgOpts.concurrency = opts.concurrency;
#endif

    setPkgOptions(pkgOpts);

    if (opts.prefix)
    {
        setenv("CPM_PREFIX", opts.prefix, 1);
        setenv("PREFIX", opts.prefix, 1);
    }

    if (opts.force)
        setenv("FORCE", "1", 1);

    int code = program.argc == 0 ? installLocalpkgs() : installPackages(program.argc, program.argv);

    curl_global_cleanup();
    cleanPkgs();
    command_free(&program);
    return code;
}

static void setDir(command_t *self)
{
    opts.dir = (char *)self->arg;
    debug(&debugger, "set dir: %s", opts.dir);
}

static void setPrefix(command_t *self)
{
    opts.prefix = (char *)self->arg;
    debug(&debugger, "set prefix: %s", opts.prefix);
}

static void setToken(command_t *self)
{
    opts.token = (char *)self->arg;
    debug(&debugger, "set token: %s", opts.token);
}
static void unsetVerbose(command_t *self)
{
    opts.verbose = 0;
    debug(&debugger, "unset verbose");
}
static void setDev(command_t *self)
{
    opts.dev = 1;
    debug(&debugger, "set develmopent flag");
}
static void setSave(command_t *self)
{
    opts.save = 1;
    debug(&debugger, "set save flag");
}

static void setSaveDev(command_t *self)
{
    opts.savedev = 1;
    debug(&debugger, "set save develmopent flag");
}

static void setForce(command_t *self)
{
    opts.force = 1;
    debug(&debugger, "set force flag");
}

static void setGlobal(command_t *self)
{
    opts.global = 1;
    debug(&debugger, "set global flag");
}

static void setSkipCache(command_t *self)
{
    opts.skipCache = 1;
    debug(&debugger, "set skip cache flag");
}

static int installLocalpkgs()
{
    const char *file = "manifest.json";
    if (fs_exists(file) == -1)
    {
        logger_error("error", "Missing manifest file");
        return 1;
    }

    debug(&debugger, "reading local manifest");
    char *json = fs_read(file);
    if (json == NULL)
        return 1;

    Package *pkg = newPkg(json, opts.verbose);
    if (pkg == NULL)
        goto e1;
    if (pkg->prefix)
        setenv("PREFIX", pkg->prefix, 1);

    int rc = installDeps(pkg, opts.dir, opts.verbose);
    if (rc == -1)
        goto e2;

    if (opts.dev)
    {
        rc = installDev(pkg, opts.dir, opts.verbose);
        if (rc == -1)
            goto e2;
    }

    free(json);
    freePkg(pkg);
    return 0;

e2:
    freePkg(pkg);
e1:
    free(json);
    return 1;
}

static int writeDeps(Package *pkg, char *prefix)
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

static int saveDeps(Package *pkg)
{
    debug(&debugger, "savind dependency %s at %s", pkg->name, pkg->version);
    return writeDeps(pkg, "dependencies");
}

static int saveDevDeps(Package *pkg)
{
    debug(&debugger, "savind dev dependency %s at %s", pkg->name, pkg->version);
    return writeDeps(pkg, "develmopent");
}

static int installPackage(const char *slug)
{
    Package *pkg = NULL;
    int rc;
    long pathMax;

#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif defined(_PC_PATH_MAX)
    pathMax = pathconf(slug, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

    if (!rootPkg)
    {
        const char *name = "manifest.json";
        char *json = fs_read(name);
        if (json)
            rootPkg = newPkg(json, opts.verbose);
    }

    if (slug[0] == '.')
        if (strlen(slug) == 1 || (slug[1] == '/' && strlen(slug) == 2))
        {
            char dir[pathMax];
            realpath(slug, dir);
            slug = dir;
            return installLocalpkgs("manifest.json");
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
            return installLocalpkgs(slug);
        }

        if (stats)
            free(stats);
    }

    if (!pkg)
        pkg = newPkgSlug(slug, opts.verbose);

    if (pkg == NULL)
        return -1;

    if (rootPkg && rootPkg->prefix)
    {
        pkgOpts.prefix = rootPkg->prefix;
        setPkgOptions(pkgOpts);
    }

    rc = installPkg(pkg, opts.dir, opts.verbose);
    if (rc != 0)
        goto clean;

    if (rc == 0 && opts.dev)
    {
        rc = installDev(pkg, opts.dir, opts.verbose);
        if (rc != 0)
            goto clean;
    }

    if (pkg->repo == 0 || strcmp(slug, pkg->repo) != 0)
        pkg->repo = strdup(slug);

    if (opts.save)
        saveDeps(pkg);
    if (opts.savedev)
        saveDevDeps(pkg);

clean:
    freePkg(pkg);
    return rc;
}

static int installPackages(int n, char **pkgs)
{
    for (int i = 0; i < n; i++)
    {
        debug(&debugger, "install %s (%d)", pkgs[i], i);
        if (installPackage(pkgs[i]) == -1)
            return 1;
    }

    return 0;
}