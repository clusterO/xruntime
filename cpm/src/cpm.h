#ifndef _CPM_HEADER_
#define _CPM_HEADER_

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#ifndef LATEST_RELEASE_URL
#define LATEST_RELEASE_URL ""
#endif

#define NOTIF_EXPIRATION 259200

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

#define format(...)                                         \
    ({                                                      \
        if (asprintf(__VA_ARGS__) == -1)                    \
        {                                                   \
            rc = 1;                                         \
            fprintf(stderr, "Memory allocation failure\n"); \
            goto clean;                                     \
        }                                                   \
    })

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

#include "common/cache.h"
#include "libs/asprintf.h"
#include "libs/debug.h"
#include "libs/fs.h"
#include "libs/http-get.h"
#include "libs/logger.h"
#include "libs/parson.h"
#include "libs/path-join.h"
#include "libs/str-flatten.h"
#include "libs/strdup.h"
#include "libs/trim.h"
#include "libs/which.h"
#include "libs/commander.h"

static const char *usage = "usage: cpm -h | --help\nusage: cpm -v | -V";

static bool checkRelease(const char *path);
static void compareVersions(const JSON_Object *res, const char *marker);
static void checkNewReleases();
static char *getFullCommand(char *cmd);
static int getCpmCommandOptions(command_t *program, char *args, char *argv, int argc);
static int executeCommand(char *cmd, char **argv, int argc);
static int executeInstalledCommand(char *cmd, char *args);

#endif