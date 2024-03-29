#ifndef _INIT_HEADER_
#define _INIT_HEADER_

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libs/asprintf.h"
#include "libs/logger.h"
#include "libs/debug.h"
#include "libs/commander.h"
#include "libs/parson.h"
#include "common/package.h"

debug_t debugger;

typedef struct
{
    char *manifest;
    int verbose;
} InitOptions;

static void setOpts(command_t *cmd);
static void setManifestOpts(command_t *cmd);
static void getInitCommandOptions(command_t *program, int argc, char **argv);
static char *basePath();
static void getInput(char *buffer, size_t s);
static void readInput(JSON_Object *root, const char *key, const char *defaultValue, const char *output);
static inline size_t writeManifest(const char *manifest, const char *str, size_t length);
static int writePackages(const char *manifest, JSON_Value *pkg);

#endif