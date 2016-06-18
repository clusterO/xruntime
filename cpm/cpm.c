#include "cpm.h"

int main(int argc, char **argv)
{
    char *cmd = argv[1];
    char *args = NULL;
    char *command = NULL;
    char *commandArgs = NULL;
    char *bin = NULL;
    int rc = 1;

    debug_init(&debugger, "cmp");

    ccInit();           // initialize the cache path
    notifyNewRelease(); // check if there is new versions

#pragma region refactor command selector

    if (cmd == NULL)
    {
        printf("%s\n", usage);
        return 0;
    }

    if (strncmp(cmd, "-v", 2) == 0)
    {
        fprintf(stderr, "Deprecated flag: \"-v\". Please use \"-V\"\n");
        cmd = "-V";
    }

    if (strncmp(cmd, "-V", 2) == 0 || strncmp(cmd, "--version", 9) == 0)
    {
        printf("%s\n", VERSION);
        return 0;
    }

    if (strncmp(cmd, "--", 2) == 0)
    {
        fprintf(stderr, "Unknown option: \"%s\"\n", cmd);
        return 1;
    }

    cmd = strdup(cmd);
    if (cmd == NULL)
    {
        fprintf(stderr, "Failed to allocate memory");
        return 1;
    }

    cmd = trim(cmd);

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0)
    {
        if (argc >= 3)
        {
            free(cmd);
            cmd = strdup(argv[2]);
            args = strdup("--help");
        }
        else
        {
            fprintf(stderr, "Help command required.\n");
            goto clean;
        }
    }
    else
    {
        if (argc >= 3)
        {
            args = str_flatten(argv, 2, argc);
            if (args == NULL)
                goto clean;
        }
    }

    debug(&debugger, "args: %s", args);

#pragma endregion

#pragma region refactor command executer

    cmd = strcmp(cmd, "i") == 0 ? strdup("install") : cmd;
    cmd = strcmp(cmd, "up") == 0 ? strdup("update") : cmd;

#pragma region This generate command from /usr/bin
#ifdef _WIN32
    format(&command, "cpm-%s.exe", cmd);
#else
    format(&command, "%s", cmd);
#endif

    debug(&debugger, "command '%s'", cmd);

    bin = which(command); // check against getenv("PATH")
    if (bin == NULL)
    {
        fprintf(stderr, "Unsupported command \"%s\"\n", cmd);
        goto clean;
    }

#ifdef _WIN32 // should move before which
    for (char *p = bin; *p; p++)
        if (*p == '/')
            *p = '\\';
#endif

    if (args)
        format(&commandArgs, "%s %s", bin, args);
    else
        format(&commandArgs, "%s", bin);

    debug(&debugger, "exec: %s", commandArgs);

#pragma endregion

    // we are calling here the command from our compilation fodler
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "cd $PWD; ./%s;", cmd);
    rc = system(buffer);
    if (WIFSIGNALED(rc) && (WTERMSIG(rc) == SIGINT || WTERMSIG(rc) == SIGQUIT))
        exit(-1);

    debug(&debugger, "returned %d", rc);

    if (rc > 255)
        rc = 1;

#pragma endregion

clean:
    free(cmd);
    free(args);
    free(command);
    free(commandArgs);
    free(bin);
    return rc;
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

static void compareVersions(const JSON_Object *res, const char *marker)
{
    const char *latestVersion = json_object_get_string(res, "tag");

    if (strcmp(VERSION, latestVersion) != 0)
        logger_info("info", "New version is available. use upgrade --tag %s", latestVersion);
}

static void notifyNewRelease()
{
    const char *marker = path_join(ccMetaPath(), "Notification checked"); // ccMetaPath get the cache path

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