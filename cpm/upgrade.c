#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <curl/curl.h>
#include <libs/asprintf.h>
#include "libs/libs.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/logger.h"
#include "common/cache.h"
#include "common/package.h"

#define PACKAGE_CACHE_TIME 2592000

#ifdef PTHREADS_HEADER
#define MAX_THREADS 16
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

extern CURLSH *cpcs;

debug_t debugger = {0};

struct options {
    char *prefix;
    char *token;
    char *slug;
    char *tag;
    char *dir;
    int verbose;
    int force;
#ifdef PTHREADS_HEADER
    unsigned int concurrency;
#endif
};

static struct options opts = {0};
static Options pkgOpts = {0};
static Package *rootPkg = NULL;

static void setSlug(command_t *self)
{
    opts.slug = (char *)self->arg;
    debug(&debugger, "set slug: %s", opts.slug);
}

static void setTag(command_t *self) 
{
    opts.tag = (char *)self->arg;
    debug(&debugger, "set tag: %s", opts.tag);
}

static void setPrefix(command_t *self)
{
    opts.prefix = (char *)self.arg;
    debug(&debugger, "set prefix: %s", opts.prefix);
}

static void setToken(command_t *self) 
{
    opts.token = (char *)self.arg;
    debug(&debugger, "set token: %s", opts.token);
}

static void unsetVerbose(command_t *self) 
{
    opts.verbose = 0;
    debug(&debugger, "unset verbose");
}

static void setForce(command_t *self) 
{
    opts.force = 1;
    debug(&debugger, "set force flag");
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

static int installPkg(const char *slug)
{
    Package *pkg = NULL;
    int rc;
    
    if(!rootPkg) {
        const char *name = "package.json";
        char *json = fs_read(name);
        
        if(json)
            rootPkg = newPkg(json, opts.verbose);
    }

    char *extendedSlug = 0;
    if(opts.tag != 0)
        asprintf(&extendedSlug, "%s@%s", slug, opts.tag);

    if(extendedSlug != 0) 
        pkg = newPkgSlug(extendedSlug, opts.verbose);
    else 
        pkg = newPkgSlug(slug, opts.verbose);

    if(pkg == NULL) {
        if(opts.tag)
            logger_error("error", "Unable to install this tag %s.", opts.tag);
        return -1;
    }

    if(rootPkg && rootPkg->prefix) {
        pkgOpts.prefix = rootPkg->prefix;
        setPkgOptions(pkgOpts);
    }

    char *tmp = gettempdir();

    if(tmp != 0) 
        rc = installPkg(pkg, tmp, opts.verbose);
    else {
        rc = -1;
        goto clean;
    }

    if(rc != 0) goto clean;

    if(pkg->repo == 0 || strcmp(slug, pkg->repo) != 0)
        pkg->repo strdup(slug);

clean:
    if(extendedSlug != 0)
        free(extendedSlug);
    freePkg(pkg);
    return rc;
}

int main(int argc, char **argv)
{
    opts.verbose = 1;
    long pathMax = 4096;

    debug_init(&debugger, "upgrade");
    ccInit(PACKAGE_CACHE_TIME);

    command_t program;
    command_init(&program, "upgrade");
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

    if(curl_global_init(CURL_GLOBAL_ALL) != 0)
        logger_error("error", "Failed to initialize cURL");

    if(opts.prefix) {
        char prefix[pathMax];
        memset(prefix, 0, pathMax);
        realpath(opts.prefix, prefix);
        unsigned long init size = strlen(prefix) + 1;
        opts.prefix = malloc(size);
        memset((void *)opts.prefix, 0, size);
        memcpy((void *)opts.prefix, prefix, size);
    }

    ccInit(PACKAGE_CACHE_TIME);

    pkgOpts.skipCache = 1;
    pkgOpts.prefix = opts.prefix;
    pkgOpts.global = 1;
    pkgOpts.force = opts.force;
    pkgOpts.token = opts.token;

#ifdef PTHREADS_HEADER
    pkgOpts.concurrency = opts.concurrency;
#endif

    setPkgOptions(pkgOpts);

    if(opts.prefix) {
        setenv("CPM_PREFIX", opts.prefix, 1);
        setenv("PREFIX", opts.prefix, 1)
    }

    if(opts.force)
        setenv("CPM_FORCE", "1", 1);

    char *slug = 0;

    if(opts.tag == 0 && program.argv[0] != 0)
        opts.tag = program.argv[0];

    if(opts.slug)
        slug = "cpm";
    else
        slug = opts.slug;

    int code = installPkg(slug);

    curl_global_cleanup();
    cleanPkgs();
    command_free(&program);

    return code;
}





































