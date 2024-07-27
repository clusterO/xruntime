#include "request.h"

// FIX - conflicting types for ‘getData’
struct Data *getData(int fd, struct Cache *cache, const char *path, char *mimeType)
{
    char filePath[4096];
    struct Data *fileData;

    // Enhance - Fetcher
    if (strcmp(path, "/") == 0) // contain html¡
    {
        snprintf(filePath, sizeof(filePath), "%s/%s", SERVER_ROOT, path); // correspendance between uri and page name
        fileData = loadFile(filePath);

        if (fileData == NULL)
            return NULL;

        mimeType = getMimeType(filePath);
    }
    else if (strcmp(path, "/data") == 0) // contain endpoint
    {
        char *body = "{'key': 'value'}";
        fileData->data = body;
        fileData->size = sizeof(char) * strlen(fileData->data);

        mimeType = "application/json";
    }
    else
        return NULL;

    cput(cache, filePath, mimeType, fileData->data, fileData->size); // add to cache
    return fileData;
}

struct Data *notFound(int fd)
{
    char *filePath = "./serverfiles/404.html";
    struct Data *fileData;
    char *mimeType;

    fileData = loadFile(filePath);

    if (fileData == NULL)
    {
        fprintf(stderr, "Cannot find system 404 file\n");
        exit(3);
    }

    mimeType = getMimeType(filePath);
    return fileData;
}

char *findBodyStart(char *header)
{
}
