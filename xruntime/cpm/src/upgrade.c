#include "upgrade.h"

debug_t debugger = {0};

static UpgradeOptions options = {0};
static Options packageOptions = {0};
static Package *rootPackage = NULL;

int main(int argc, char **argv)
{
    command_t program;
    options.verbose = 1;
    long pathMax = 4096;
    char *slug = 0;

    debug_init(&debugger, "upgrade");
    createCache(PACKAGE_CACHE_TIME);
    getUpgradeCommandOptions(&program, argc, argv);
    debug(&debugger, "%d arguments", program.argc);

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        logger_error("error", "Failed to initialize cURL");

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

    packageOptions.skipCache = 1;
    packageOptions.prefix = options.prefix;
    packageOptions.global = 1;
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
        setenv("CPM_FORCE", "1", 1);

    if (options.tag == 0 && program.argv[0] != 0)
        options.tag = program.argv[0];

    if (options.slug)
        slug = "cpm";
    else
        slug = options.slug;

    int code = installPackage(slug);

    curl_global_cleanup();
    cleanPkgs();
    command_free(&program);

    return code;
}

static void getUpgradeCommandOptions(command_t *program, int argc, char **argv)
{
    command_init(program, "upgrade", VERSION);
    program->usage = "[options] [name <>]";

    command_option(program, "-P", "--prefix <dir>", "Change the prefix directory (default '/usr/local')", setPrefix);
    command_option(program, "-q", "--quiet", "Disable verbose", unsetVerbose);
    command_option(program, "-f", "--force", "Force action", setForce);
    command_option(program, "-t", "--token <token>", "Access token", setToken);
    command_option(program, "-S", "--slug <slug>", "Project path", setSlug);
    command_option(program, "-T", "--tag <tag>", "The tag to upgrade to 'default: latest'", setTag);
#ifdef PTHREADS_HEADER
    command_option(&program, "-C", "--concurrency <number>", "Set concurrency 'default: " S(MAX_THREADS) "'", setConcurrency);
#endif

    command_parse(&program, argc, argv);
}

static int installPackage(const char *slug)
{
    Package *package = NULL;
    int rc;

    if (!rootPackage)
    {
        const char *name = "package.json";
        char *json = fs_read(name);

        if (json)
            rootPackage = newPackage(json, options.verbose);
    }

    char *extendedSlug = 0;
    if (options.tag != 0)
        asprintf(&extendedSlug, "%s@%s", slug, options.tag);

    if (extendedSlug != 0)
        package = newPackageSlug(extendedSlug, options.verbose);
    else
        package = newPackageSlug(slug, options.verbose);

    if (package == NULL)
    {
        if (options.tag)
            logger_error("error", "Unable to install this tag %s.", options.tag);
        return -1;
    }

    if (rootPackage && rootPackage->prefix)
    {
        packageOptions.prefix = rootPackage->prefix;
        setPackageOptions(packageOptions);
    }

    char *tmp = gettempdir();

    if (tmp != 0)
        rc = installRootPackage(package, tmp, options.verbose);
    else
    {
        rc = -1;
        goto clean;
    }

    if (rc != 0)
        goto clean;

    if (package->repo == 0 || strcmp(slug, package->repo) != 0)
        package->repo = strdup(slug);

clean:
    if (extendedSlug != 0)
        free(extendedSlug);

    freePackage(package);

    return rc;
}

static void setSlug(command_t *self)
{
    options.slug = (char *)self->arg;
    debug(&debugger, "set slug: %s", options.slug);
}

static void setTag(command_t *self)
{
    options.tag = (char *)self->arg;
    debug(&debugger, "set tag: %s", options.tag);
}
