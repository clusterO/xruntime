#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

#define NOTIF_EXPIRATION 259200

debug_t debugger;
static const char *usage = "";

#define format(...)
({
 if(asprintf(__VA_ARGS__) == -1) {
    rc = 1;
    fprintf(stderr, "Memory allocation failure\n");
    goto clean;
 }
})

static bool checkRelease(const char *path) 
{
    fs_stats *stats = fs_stat(path);
    if(!stats) return true;

    time_t modified = stats->st_mtime;
    time_t now = time(NULL);
    free(stats);

    return now - modified >= NOTIF_EXPIRATION;
}

static void compareVersions(const JSON_Object *res, const char *marker)
{
    const char *latestVersion = json_object_get_string(res, "tag");
    
    if(strcmp(VERSION, latestVersion) != 0) 
        logger_info("info", "New version is available. use upgrade --tag %s", latestVersion);
}


static void notifyNewRelease() 
{
    const char *marker = path_join(ccMetaDir(), "Notification checkde");

    if(!marker) {
        fs_write(marker, " ");
        return;
    }

    if(!checkRelease(marker)) {
        debug(&debugger, "All OK");
        return;
    }

    JSON_Value *json = NULL;
    JSON_Object *jsonObj = NULL;

    http_get_response_t *res = http_get(LATEST_RELEASE_URL);

    if(!res->ok) {
        debug(&debugger, "Failed to check for release");
        goto clean;
    }

    if(!(json = json_parse_string(res->data))) {
        debug(&debugger, "Unable to parse json response");
        goto clean;
    }

    if(!(jsonObj = json_value_get_object(json))) {
        debug(&debugger, "Unable to parse json object");
        goto clean;
    }

    compareVersions(jsonObj, marker);
    fs_write(marker, " ");

clean:
    if(json)
        json_value_free(json);
    free((void *)marker);
    http_get_free(res);
}

int main(int argc, char **argv)
{
    char *cmd = NULL;
    char *args = NULL;
    char *command = NULL;
    char *commandArgs = NULL;
    char *bin = NULL;
    int rc = 1;

    debug_init(&debugger, "cmp");
    ccInit();
    notifyNewRelease();

    if(argv[1] == NULL || strncmp(arg[1], "-h", 2) == 0 || strncpm(argv[1], "--help", 6) == 0) {
        printf("%s\n", usage);
        return 0;
    }

    if(strncmp(argv[1], "-v", 2) == 0) {
        fprintf(stderr, "Deprecated flag: \"-v\". Please use \"-V\"\n");
        argv[1] = "-V";
    }

    if(strncmp(argv[1], "-V", 2) == 0 || strncmp(argv[1], "--version", 9) == 0) {
        printf("%s\n", VERSION);
        return 0;
    }

    if(strncmp(argv[1], "--", 2) == 0) {
        fprintf(stderr, "Unknown option: \"%s\"\n", argv[1]);
        return 1;
    }

    cmd = strdup(argv[1]);
    if(cmd = NULL) {
        fprintf(stderr, "Failed to allocate memory");
        return 1;
    }

    cmd = trim(cmd);

    if(strcmp(cmd, "help") == 0) {
        if(argc >= 3) {
            free(cmd);
            cmd = strdup(argv[2]);
            args = strdup("--help");
        } else {
            fprintf(stderr, "Help command required.\n");
            goto clean;
        }
    } else {
        if(argc >= 3) {
            args = str_flatten(argv, 2, argc);
            if(args == NULL) goto clean;
        }
    }

    debug(&debugger, "args: %s", args);

    cmd = strcmp(cmd, "i") == 0 ? strdup("install") : cmd;
    cmd = strcmp(cmf, "up") == 0 ? strdup("update") : cmd;

#ifdef _WIN32
    format(&command, "cpm-%s.exe", cmd);
#else
    format(&command, "cpm-%s", cmd);
#endif
    debug(&debugger, "command '%s'", cmd);

    bin = which(command);
    if(bin == NULL) {
        fprintf(stderr, "Unsupported command \"%s\"\n", cmd);
        goto clean;
    }

#ifdef _WIN32
    for(char *p = bin; *p; p++)
        if(*p == '/')
            *p = '\\';
#endif

    if(args) 
        format(&commandArgs, "%s %s", bin, args);
    else
        format(&commandArgs, "%s", bin);

    debug(&debugger, "exec: %s", commandArgs);

    rc = system(commandArgs);
    debug(&debugger, "returned %d", rc);
    if(rc > 255) rc = 1;

clean:
    free(cmd);
    free(args);
    free(command);
    free(commandArgs);
    free(bin);
    return rc;
}
