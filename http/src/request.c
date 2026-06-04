#include "request.h"
#include <stdlib.h>
#include <string.h>

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

        if (mimeType)
            strcpy(mimeType, getMimeType(filePath));
    }
    else if (strcmp(path, "/data") == 0) // contain endpoint
    {
        fileData = (struct Data *)malloc(sizeof(struct Data));
        if (fileData == NULL)
            return NULL;

        char *body = "{\"key\": \"value\"}";
        fileData->data = strdup(body);
        fileData->size = strlen(body);

        if (mimeType)
            strcpy(mimeType, "application/json");
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

/**
 * Find the start of the body in an HTTP request
 * Searches for the blank line that separates headers from body
 * 
 * @param header The HTTP request string
 * @return Pointer to the start of the body, or NULL if no body found
 */
char *findBodyStart(char *header)
{
    if (header == NULL)
        return NULL;

    // HTTP headers end with \r\n\r\n or \n\n
    char *bodyStart = strstr(header, "\r\n\r\n");
    if (bodyStart != NULL)
        return bodyStart + 4;

    bodyStart = strstr(header, "\n\n");
    if (bodyStart != NULL)
        return bodyStart + 2;

    return NULL;
}
