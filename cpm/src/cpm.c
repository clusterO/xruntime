#include "cpm.h"

debug_t debugger;

int main(int argc, char **argv)
{
    command_t program;
    char *cmd = argv[1];
    char *args = NULL;
    int rc = 1;

    debug_init(&debugger, "cmp");
    initCache();
    checkNewReleases();
    getCpmCommandOptions(&program, args, argv, argc);
    executeCommand(cmd, argv, argc); // cmd to be replaced with program

    if (rc > 255)
        rc = 1;

    return rc;
}

static void compareVersions(const JSON_Object *res, const char *marker)
{
    const char *latestVersion = json_object_get_string(res, "tag");

    if (strcmp(VERSION, latestVersion) != 0)
        logger_info("info", "New version is available. use upgrade --tag %s", latestVersion);
}

static int getCpmCommandOptions(command_t *program, char *args, char *argv, int argc)
{
    command_init(program, "cpm", VERSION);
    program->usage = "[options] [name <>]";

    command_option(program, NULL, NULL, "", printf("%s\n", usage));
    command_option(program, "-v", "version", "Show version", printf("VERSION: %s\n", VERSION));
    command_option(program, "-V", "version", "Show version", printf("VERSION: %s\n", VERSION));
    command_option(program, "-b", "build", "Build packages", NULL);
    command_option(program, "-c", "config", "Configure CPM", NULL);
    command_option(program, "-i", "install  <package | .>", "Install packages", NULL);
    command_option(program, "-s", "search <package>", "Search if a package exists", NULL);
    command_option(program, "-u", "update <package | .>", "Update packages to the latest version", NULL);
    command_option(program, "-g", "upgrade <package | .>", "Upgrade packages", NULL);
#ifdef PTHREADS_HEADER
    command_option(program, "-C", "--concurrency", "Set concurrency <number>", setConcurrency);
#endif

    command_parse(program, argc, argv);

    if (argc >= 3)
    {
        args = str_flatten(argv, 2, argc);
        if (args == NULL)
            return -1;
    }

    debug(&debugger, "args: %s", args);

    return 1;
}

static void checkNewReleases()
{
    const char *marker = path_join(cacheMetaPath(), "Notification checked");

    if (!marker)
    {
        fs_write(marker, " ");
        return;
    }

    if (!checkRelease(marker))
    {
        debug(&debugger, "All OK");
        return;
    }

    JSON_Value *json = NULL;
    JSON_Object *jsonObj = NULL;

    http_get_response_t *res = http_get(LATEST_RELEASE_URL);

    if (!res->ok)
    {
        debug(&debugger, "Failed to check for release");
        goto clean;
    }

    if (!(json = json_parse_string(res->data)))
    {
        debug(&debugger, "Unable to parse json response");
        goto clean;
    }

    if (!(jsonObj = json_value_get_object(json)))
    {
        debug(&debugger, "Unable to parse json object");
        goto clean;
    }

    compareVersions(jsonObj, marker);
    fs_write(marker, " ");

clean:
    if (json)
        json_value_free(json);

    free((void *)marker);
    http_get_free(res);
}

static bool checkRelease(const char *path)
{
    fs_stats *stats = fs_stat(path);
    if (!stats)
        return true;

    time_t modified = stats->st_mtime;
    time_t now = time(NULL);
    free(stats);

    return now - modified >= NOTIF_EXPIRATION;
}

static int executeCommand(char *cmd, char **argv, int argc)
{
    for (int i = 2; i < argc; i++)
    {
        // doesn't concat args when full command used
        strcat(cmd, " ");
        strcat(cmd, argv[i]);
    }

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "cd $PWD; ./%s;", cmd);
    int rc = system(buffer);
    if (WIFSIGNALED(rc) && (WTERMSIG(rc) == SIGINT || WTERMSIG(rc) == SIGQUIT))
        exit(-1);

    debug(&debugger, "returned %d", rc);

    return rc;
}

static int executeInstalledCommand(char *cmd, char *args)
{
    char *command = NULL;
    char *commandArgs = NULL;
    char *bin = NULL;
    int rc = 1;

#ifdef _WIN32
    format(&command, "cpm-%s.exe", cmd);
#else
    format(&command, "%s", cmd);
#endif

    debug(&debugger, "command '%s'", cmd);

    bin = which(command);
    if (bin == NULL)
    {
        fprintf(stderr, "Unsupported command \"%s\"\n", cmd);
        goto clean;
    }

#ifdef _WIN32
    for (char *p = bin; *p; p++)
        if (*p == '/')
            *p = '\\';
#endif

    if (args)
        format(&commandArgs, "%s %s", bin, args);
    else
        format(&commandArgs, "%s", bin);

    debug(&debugger, "exec: %s", commandArgs);

clean:
    free(command);
    free(commandArgs);
    free(bin);
}
