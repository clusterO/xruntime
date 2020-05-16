#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cache.h"
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


