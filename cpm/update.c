#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <curl/curl.h>
#include "libs/debug.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/logger.h"
#include "libs/commander.h"
#include "libs/parson.h"
#include "package.h"
#include "cache.h"

#define PACKAGE_CACHE_TIME 2592000

#ifdef PTHREADS_HEADER
#define MAX_THREADS 12
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

debug_t debugger = {0};

struct options {
    char const *dir;
    char *prefix;
    char *token;
    char verbose;
    int dev;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
};

static struct options opts = {0};
static Options pkgOpts = {0};
static Package *rootPkg = NULL;

static void setDir(command_t *self)
{
    opts.dir = (char *)self->arg;
    debug(&debugger, "set dir: %s", opts.dir);
}

static void setToken(command_t *self)
{
    opts.token = (char *)self->arg;
    debug(&debugger, "set token: %s", opts.token);
}

static void setPrefix(command_t *self)
{
    opts.prefix = (char *)self->arg;
    debug(&debugger, "set prefix: %s", opts.prefix);
}

static void unsetVerbose(command_t *self)
{
    opts.verbose = 0;
    debug(&debugger, "unset verbose");
}

static setDev(command_t *self)
{
    opts.dev = 1;
    debug(&debugger, "set development flag");
}

#ifdef PTHREADS_HEADER
static void setConcurrency(command_t *self) 
{
    if(self->arg) {
        opts.concurrency = atol(self->arg);
        debug(&debugger, "set concurrency: %lu", opts.concurrency);
    } 
}
#endif

static int installLocalPkgs()
{
    if(fs_exists("manifest.json") == -1) {
        logger_error("error", "Missing config file");
        return 1;
    }

    debug(&debugger, "reading local config file");
    char *json = fs_read(file);
    if(json == NULL) return 1;

    Package *pkg = newPkg(json, opts.verbose);
    if(pkg == NULL) goto e1;
    if(pkg->prefix) setenv("PREFIX", pkg->prefix, 1);

    int rc = installDeps(pkg, opts.dir, opts.verbose);
    if(rc == -1) goto e2;

    if(opts.dev) {
        rc = installDev(pkg, opts.dir, opts.verbose);
        if(rc == -1) goto e2;
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

    if(pkgJson == NULL || pkgJsonObj == NULL) return 1;

    JSON_Object *dep = json_object_dotget_object(pkgJsonObj, prefix);
    if(dep == NULL) {
        newDep = json_value_init_object();
        dep = json_value_get_object(newDep);
        json_object_set_value(pkgJsonObj, prefix, newDep);
    }

    json_object_set_string(dep, pkg->repo, pkg->version);

    int rc = json_serialize_to_file_pretty(pkgJson, file);
    json_value_free(pkgJson);
    return rc;
}

static int installPkg(const char *slug) 
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

    if(!rootPkg) {
        const char *name = "manifest.json";
        char *json = fs_read(name);
        
        if(json)
            rootPkg = newPkg(json, opts.verbose);
    }
    
    if(slug[0] == '.')
        if(strlen(slug) == 1 || (slug[1] == '/' && strlen(slug))) {
            char dir[pathMax];
            realpath(slug, dir);
            slug = dir;
            return installLocalPkgs();
        }

    if(fs_exists(slug) == 0) {
        fs_stats *stats = fs_stat(slug);
        if(stats != NULL && (S_IFREG == (stats->st_mode & S_IFMT)
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION)
                    || S_IFLNK == (stats->st_mode & S_IFMT)
#endif
                    )) {
            free(stats);
            return installLocalPkgs();
        }

        if(stats) free(stats);
    }

    if(!pkg)
        pkg = newPkgSlug(slug, opts.verbose);

    if(pkg == NULL) return -1;

    if(rootPkg && rootPkg->prefix) {
        pkgOpts.prefix = rootPkg->prefix;
        setPkgOptions(pkgOpts);
    }

    rc = installPkg(pkg, opts.dir, opts.verbose);
    if(rc != 0) goto clean;

    if(rc == 0 && opts.dev) {
        rc = installDev(pkg, opts.dir, opts.verbose);
        if(rc != 0) goto clean;
    }

    if(pkg->repo == 0 || strcmp(slug, pkg->repo) != 0)
        pkg->repo = strdup(slug);

clean:
    freePkg(pkg);
    return rc;
}

static int installPackages(int n, char **pkg)
{
    for(int i = 0; i < n; i++) {
        debug(&debugger, "install %s (%d)", pkgs[i], i);
        if(installPkg(pkgs[i]) == -1) return 1;
    }

    return 0;
}

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
#elif
    pathMax = pathconf(opts.dir, _PC_PATH_MAX);
#else
    pathMax = 4096;
#endif

    debug_init(&debugger, "update");
    ccInit(PACKAGE_CACHE_TIME);

    command_t program;
    command_init(&program, "update");
    program.usage = "[options] [name <>]";
    
    command_option(&program, "-o", "--out <dir>", "Change the output directory 'default: deps'", setDir);
    command_option(&program, "-P", "--prefix <dir>", "Change the prefix directory 'default: /usr/local'", setPrefix);
    command_option(&program, "-q", "--quiet", "Disable verbose");
    command_option(&program, "-d", "--dev", "Install development dependencies", setDev);
    command_option(&program, "-t", "--token <token>", "Set access token", setToken);
#ifdef
    command_option(&program, "-C", "--concurrency", "Set concurrency <number>", setConcurrency);
#endif
    command_parse(&program, argc, argv);

    debug(&debugger, "%d arguments", program.argc);

    if(curl_global_init(CURL_GLOBAL_ALL) != 0) 
        logger_error("error", "Failed to init cURL");

    if(opts.prefix) {
        char prefix[pathMax];
        memset(prefix, 0, pathMax);
        realpath(opts.prefix, prefix);
        unsigned long int size = strlen(prefix) + 1;
        opts.prefix = malloc(size);
        memset((void *)opts.prefix, 0, size);
        memcpy((void *)opts.prefix, prefix, size);
    }

    ccInit(PACKAGE_CACHE_TIME);

    pkgOpts.skipCache = 1;
    pkgOpts.prefix = opts.prefix;
    pkgOpts.global = 0;
    pkgOpts.force = 1;
    pkgOpts.token = opts.token;

#ifdef PTHREADS_HEADER
    pkgOpts.concurrency = opts.concurrency;
#endif

    setPkgOptions(pkgOpts);

    if(opts.prefix) {
        setenv("CPM_PREFIX", opts.prefix, 1);
        setenv("PREFIX", opts.prefix, 1);
    }

    setenv("FORCE", "1", 1);

    int code = program.argc == 0 ? installLocalPkgs() : installPackages(program.argc, program.argv);

    curl_global_cleanup();
    cleanPkgs();
    command_free(program);
    return code;
}

































