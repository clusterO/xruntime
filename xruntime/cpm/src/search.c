#include "search.h"

debug_t debugger;

static int color;
static int cache;
static int json;

int main(int argc, char **argv)
{
    color = 1;
    cache = 1;

    debug_init(&debugger, "search");
    createCache(SEARCH_CACHE_TIME);

    command_t program;
    getSearchCommandOptions(&program, argc, argv);

    for (int i = 0; i < program.argc; i++)
        case_lower(program.argv[i]);

    cc_color_t highlightColor = color ? CC_FG_DARK_GREEN : CC_FG_NONE;
    cc_color_t textColor = color ? CC_FG_GRAY : CC_FG_NONE;

    char *html = cacheSearch(); // Search NPM !!
    if (html == NULL)
    {
        command_free(&program);
        logger_error("error", "Failed to fetch HTML");
        return 1;
    }

    // need FIX
    list_t *packages = wiki_registry_parse(html);
    free(html);

    debug(&debugger, "found %zu packages", packages->len);

    list_node_t *node;
    list_iterator_t *iterator = list_iterator_new(packages, LIST_HEAD);
    JSON_Array *jsonList = NULL;
    JSON_Value *jsonListRoot = NULL;

    if (json)
    {
        jsonListRoot = json_value_init_array();
        jsonList = json_value_get_array(jsonListRoot);
    }

    while ((node = list_iterator_next(iterator)))
    {
        wiki_package_t *package = (wiki_package_t *)node->val;

        if (matches(program.argc, program.argv, package))
            if (json)
                addPkgToJson(package, jsonList);
            else
                showPkg(package, highlightColor, textColor);
        else
            debug(&debugger, "skipped package %s", package->repo);

        wiki_package_free(package);
    }

    if (json)
    {
        char *serialized = json_serialize_to_string_pretty(jsonListRoot);
        puts(serialized);
        json_free_serialized_string(serialized);
        json_value_free(jsonListRoot);
    }

    list_iterator_destroy(iterator);
    list_destroy(packages);
    command_free(&program);

    return 0;
}

static void unsetColor()
{
    color = 0;
}

static void unsetCache()
{
    cache = 0;
}

static void setJson()
{
    json = 1;
}

static int matches(int count, char *args[], wiki_package_t *package)
{
    if (count == 0)
        return 1;

    char *description = NULL;
    char *name = NULL;
    char *repo = NULL;
    char *href = NULL;
    int rc = 0;

    name = packageName(package->repo);
    COMPARE(name);
    description = strdup(package->description);
    COMPARE(description);
    repo = strdup(package->repo);
    COMPARE(repo);
    href = strdup(package->href);
    COMPARE(href);

clean:
    free(name);
    free(description);
    return rc;
}

static char *cacheSearch()
{
    if (cacheSearchExists() && cache)
    {
        char *data = getCacheSearch();
        if (data)
            return data;

        goto setCache;
    }

setCache:
    debug(&debugger, "Setting cache from %s", NPM_URL);

    http_get_response_t *res = http_get(NPM_URL);
    if (!res->ok)
        return NULL;

    char *html = strdup(res->data);
    if (html == NULL)
        return NULL;

    http_get_free(res);

    setCacheSearch(html);
    debug(&debugger, "Save cache");

    return html;
}

static void showPkg(const wiki_package_t *pkg, cc_color_t highlightColor, cc_color_t textColor)
{
    cc_fprintf(highlightColor, stdout, " %s\n", pkg->repo);
    printf("  url: ");
    cc_fprintf(textColor, stdout, "%s\n", pkg->href);
    printf("  desc: ");
    cc_fprintf(textColor, stdout, "%s\n", pkg->description);
    printf("\n");
}

static void addPkgToJson(const wiki_package_t *pkg, JSON_Array *jsonList)
{
    JSON_Value *jsonPkgRoot = json_value_init_object();
    JSON_Object *jsonPkg = json_value_get_object(jsonPkgRoot);

    json_object_set_string(jsonPkg, "repo", pkg->repo);
    json_object_set_string(jsonPkg, "href", pkg->href);
    json_object_set_string(jsonPkg, "description", pkg->description);
    json_object_set_string(jsonPkg, "category", pkg->category);
    json_array_append_value(jsonList, jsonPkgRoot);
}

static void getSearchCommandOptions(command_t *program, int argc, char **argv)
{
    command_init(program, "search", VERSION);
    program->usage = "[options] [name <>]";

    command_option(program, "-n", "--no-color", "No color", unsetColor);
    command_option(program, "-c", "--skip-cache", "Skipt cache search", unsetCache);
    command_option(program, "-j", "--json", "Generate JSON output", setJson);

    command_parse(program, argc, argv);
}
