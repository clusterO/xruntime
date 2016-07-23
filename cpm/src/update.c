#include "update.h"

debug_t debugger = {0};

static UpdateOptions options = {0};
static Options packageOptions = {0};
static Package *rootPackage = NULL;

int main(int argc, char **argv)
{
    command_t program;
    long pathMax;
    int code;

    options.verbose = 1;
    options.dev = 0;
#ifdef _WIN32
    options.dir = ".\\deps";
#else
    options.dir = "./deps";
#endif

#ifdef PATH_MAX
    pathMax = PATH_MAX;
#elif
    pathMax = pathconf(options.dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

    debug_init(&debugger, "update");
    createCache(PACKAGE_CACHE_TIME); // shouldn't be declared once at CPM or Init!
    getUpdateCommandOptions(&program, argc, argv);
    debug(&debugger, "%d arguments", program.argc);

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        logger_error("error", "Failed to init cURL");

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
    packageOptions.global = 0;
    packageOptions.force = 1;
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

    setenv("FORCE", "1", 1);

    if (program.argc == 0)
        code = installLocalPackages();
    else
        code = installPackages(program.argc, program.argv);

    curl_global_cleanup();
    cleanPkgs();
    command_free(&program);

    return code;
}

static void getUpdateCommandOptions(command_t *program, int argc, char **argv)
{
    command_init(program, "update", VERSION);
    program->usage = "[options] [name <>]";

    command_option(program, "-o", "--out <dir>", "Change the output directory 'default: deps'", setDir);
    command_option(program, "-P", "--prefix <dir>", "Change the prefix directory 'default: /usr/local'", setPrefix);
    command_option(program, "-q", "--quiet", "Disable verbose", unsetVerbose);
    command_option(program, "-d", "--dev", "Install development dependencies", setDev);
    command_option(program, "-t", "--token <token>", "Set access token", setToken);
#ifdef PTHREADS_HEADER
    command_option(program, "-C", "--concurrency", "Set concurrency <number>", setConcurrency);
#endif

    command_parse(program, argc, argv);
}
