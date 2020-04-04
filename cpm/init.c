#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "package.h"
#include "libs/asprintf.h"
#include "libs/logger.h"
#include "libs/debug.h"
#include "libs/commander.h"
#if defined(_WIN32 || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__))
#define setenv(n, v, o) _putenv(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

debug_t debugger;

struct Options {
    char *manifest;
    int verbose;
}

static struct Options opts;

static void setOpts(command_t *cmd) 
{
    opts.verbose = 0;
    debug(&debugger, "set quiet flag");
}

static void setManifestOpts(command_t *cmd) 
{
    opts.manifest = (char *)cmd->arg;
    debug(&debugger, "set manifest: %s", opts.manifest);
}


