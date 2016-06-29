#include "upgrade.h"

static UpgradeOptions options = {0};
static Options packageOptions = {0};
static Package *rootPackage = NULL;

int main(int argc, char **argv)
{
    options.verbose = 1;
    long pathMax = 4096;

    debug_init(&debugger, "upgrade");
    ccInit(PACKAGE_CACHE_TIME);

    // move to commun commander
    command_t program;
    command_init(&program, "upgrade", VERSION);
    program.usage = "[options] [name <>]";

    command_option(&program, "-P", "--prefix <dir>", "Change the prefix directory (default '/usr/local')", setPrefix);
    command_option(&program, "-q", "--quiet", "Disable verbose", unsetVerbose);
    command_option(&program, "-f", "--force", "Force action", setForce);
    command_option(&program, "-t", "--token <token>", "Access token", setToken);
    command_option(&program, "-S", "--slug <slug>", "Project path", setSlug);
    command_option(&program, "-T", "--tag <tag>", "The tag to upgrade to 'default: latest'", setTag);

#ifdef PTHREADS_HEADER
    command_option(&program, "-C", "--concurrency <number>", "Set concurrency 'default: " S(MAX_THREADS) "'", setConcurrency);
#endif
    command_parse(&program, argc, argv);

    debug(&debugger, "%d arguments", program.argc);

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        logger_error("error", "Failed to initialize cURL");

    // DRY
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

    ccInit(PACKAGE_CACHE_TIME);

    packageOptions.skipCache = 1;
    packageOptions.prefix = options.prefix;
    packageOptions.global = 1;
    packageOptions.force = options.force;
    packageOptions.token = options.token;

#ifdef PTHREADS_HEADER
    packageOptions.concurrency = options.concurrency;
#endif

    setPkgOptions(packageOptions);

    if (options.prefix)
    {
        setenv("CPM_PREFIX", options.prefix, 1);
        setenv("PREFIX", options.prefix, 1);
    }

    if (options.force)
        setenv("CPM_FORCE", "1", 1);

    char *slug = 0;

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

// DRY these functions
static int installPackage(const char *slug)
{
    Package *pkg = NULL;
    int rc;

    if (!rootPackage)
    {
        const char *name = "package.json";
        char *json = fs_read(name);

        if (json)
            rootPackage = newPkg(json, options.verbose);
    }

    char *extendedSlug = 0;
    if (options.tag != 0)
        asprintf(&extendedSlug, "%s@%s", slug, options.tag);

    if (extendedSlug != 0)
        pkg = newPkgSlug(extendedSlug, options.verbose);
    else
        pkg = newPkgSlug(slug, options.verbose);

    if (pkg == NULL)
    {
        if (options.tag)
            logger_error("error", "Unable to install this tag %s.", options.tag);
        return -1;
    }

    if (rootPackage && rootPackage->prefix)
    {
        packageOptions.prefix = rootPackage->prefix;
        setPkgOptions(packageOptions);
    }

    char *tmp = gettempdir();

    if (tmp != 0)
        rc = installPkg(pkg, tmp, options.verbose);
    else
    {
        rc = -1;
        goto clean;
    }

    if (rc != 0)
        goto clean;

    if (pkg->repo == 0 || strcmp(slug, pkg->repo) != 0)
        pkg->repo = strdup(slug);

clean:
    if (extendedSlug != 0)
        free(extendedSlug);
    freePkg(pkg);
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

static void setForce(command_t *self)
{
    options.force = 1;
    debug(&debugger, "set force flag");
}