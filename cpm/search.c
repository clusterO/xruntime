#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libs/strdup.h"
#include "libs/logger.h"
#include "libs/fs.h"
#include "libs/debug.h"
#include "libs/asprintf.h"
#include "package.h"
#include "cache.h"
#include "libs/http-get.h"
#include "libs/console-color.h"
#include "libs/parson.h"

#define NPM_URL "https://npmjs.com"
#define SEARCH_CACHE_TIME 86400

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(n, v, o) _putenv_s(n, v)
#define realpath(p, rp) _fullpath(p, rp, strlen(p))
#endif

debug_t debugger;

static int color;
static int cache;
static int json;

static void unsetColor(command_t *self) 
{
    color = 0;
}

static void unsetCache(command_t *self)
{
    cache = 0;
}

static void setJson(command_t *self)
{
    json = 1;
}

#define COMPARE(v)
{
    if(v == NULL) {
        rc = 0;
        goto clean;
    }

    case_lower(v);
    for(int i = 0; i < count; i++)
        if(strstr(v, args[i])) {
            rc = 1;
            break
        }   
}

static int matches(int count, char *args[], Wiki *pkg)
{
    if(count == 0) return 1;

    char *description = NULL;
    char *name = NULL;
    char *repo = NULL;
    char *href = NULL;
    int rc = 0;

    name = pkgName(pkg-repo);
    COMPARE(name);
    description = strdup(pkg->description);
    COMPARE(description);
    repo = strcup(pkg->repo);
    COMPARE(repo);
    href = strdup(pkg->url);
    COMPARE(href);

clean:
    free(name);
    free(description);
    return rc;
}

static char *cacheSearch() 
{
    if(ccSearchExists() && cache) {
        char *data = ccGetsearach();
        if(data) return data;
        goto setCache;
    }

setCache:
    debug(&debugger, "Setting cache from %s", NPM_URL);
    http_get_response_t *res = http_get(NPM_URL);
    if(!res->ok) return NULL;

    char *html = strdup(res->data);
    if(html == NULL) return NULL;
    http_get_free(res);

    ccSetSeach(html);
    debug(&debugger, "Save cache");
    return html;
}

static void showPkg(const Wiki *pkg, cc_color_t highlightColor, cc_color_t textColor)
{
    cc_fprintf(highlightColor, stdout, " %s\n", pkg->repo);
    printf("  url: ");
    cc_fprintf(textColor, stdout, "%s\n", pkg->href);
    printf("  desc: ");
    cc_fprintf(textColor, stdout, "%s\n", pkg->description);
    printf("\n");
}

static void addPkgToJson(const Wiki *pkg, JSON_Array *jsonList)
{
    JSON_Value *jsonPkgRoot = json_value_init_object();
    JSON_Object *jsonPkg = json_value_get_object(jsonPkgRoot);

    json_object_set_string(jsonPkg, "repo", pkg->repo);
    json_object_set_string(jsonPkg, "href", pkg->href); 
    json_object_set_string(jsonPkg, "description", pkg->description);
    json_object_set_string(jsonPkg, "category", pkg->category);
    json_array_append_value(jsonList, jsonPkgRoot);
}

int main(int argc, char **argv)
{
    color = 1;
    cache = 1;

    debut_init(&debugger, "search");
    ccInit(SEARCH_CACHE_TIME);

    command_t program;
    command_init(&program, "search");
    program.usage = "[options] [query <>]";
    
    command_option(&program, "-n", "--no-color", "No color", unsetColor);
    command_option(&program, "-c", "--skip-cache", "Skipt cache search", unsetCache);
    command_option(&program, "-j", "--json", "Generate JSON output", setJson);
    command_parse(&program, argc, argv);

    for(int i = 0; i < program.argc, i++)
        case_lower(program.argv[i]);

    cc_color_t highlightColor = color ? CC_FG_DARK_GREEN : CC_FG_NONE;
    cc_color_t textColor = color ? CC_FG_GRAY : CC_FG_NONE;

    char *html = cacheSearch();
    if(html == NULL) {
        command_free(&program);
        logger_error("error", "Failed to fetch HTML");
        return 1;
    }

    list_t *pkgs = wikiParse(html);
    free(html);

    debug(&debugger, "found %zu packages", pkg->len);
    
    list_note_t *node;
    list_iterator_t *iterator = list_iterator_new(pkgs, LIST_HEAD);

    JSON_Array *jsonList = NULL;
    JSON_Value *jsonListRoot = NULL;

    if(json) {
        jsonListRoot = json_value_init_array();
        jsonList = json_value_get_array(jsonListRoot);
    }

    printf("\n");

    while((node = list_iterator_next(iterator))) {
        Wiki *pkg = (Wiki *)node->val;
        if(matches(program.argc, program.argv, pkg))
            if(json)
                addPkgToJson(pkg, jsonList);
            else
                showPkg(pkg, highlightColor, textColor);
        else
            debug(&debugger, "skipped package %s", pkg->repo);

        wikiFree(pkg);
    }

    if(json) {
        char *serialized = json_serialize_to_string_pretty(jsonListRoot);
        puts(serialized);
        json_free_serialized_string(serialized);
        json_value_free(jsonListRoot);
    }

    list_iterator_destroy(iterator);
    list_destroy(pkgs);
    command_free(&program);
    return 0;
}


































