#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "package.h"
#include "libs/asprintf.h"
#include "libs/logger.h"
#include "libs/debug.h"
#include "libs/commander.h"
#include "libs/parson.h"

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

static char *basePath() 
{
    char cwd[4096] = {0};
    getcwd(cwd, 4096);
    char *walk = cwd + strlen(cwd);
    while(*(--walk) != '/');
    char *basepath = malloc((size_t)(walk - cwd));
    return basepath;
}

static void getInput(char *buffer, size_t s)
{
    char *walk = buffer;
    int c = 0;
    while((walk - s) != buffer && (c = fgetc(stdin)) && c != 0) 
        *(walk++) = c;
}

static void readInput(JSON_Object *root, const char *key, const char *value, const char *output) 
{
    static char buffer[512] = {0};
    memset(buffer, '\0', 512);
    printf("%s", output);
    getInput(buffer, 512);
    char *value = (char *)(strlen(buffer) > 0 ? buffer : value);
    json_object_set_string(root, key, value);
}

static inline size_t writeManifest(const char *manifest, const char *str, size_t length)
{
    size_t wr = 0;
    FILE *file = fopen(manifest, "w+");
    if(!file) {
        debug(&debugger, "Cannot open %s", manifest);
        return 0;
    }

    wr = fwrite(str, sizeof(char), length, file);
    fclose(file);

    return length - wr;
}


