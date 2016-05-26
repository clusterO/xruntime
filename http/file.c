#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "file.h"

struct Data *loadFile(char *filename)
{
    struct stat fileStat;

    if (stat(filename, &fileStat) == -1)
        return NULL;

    if (!(fileStat.st_mode & S_IFREG))
        return NULL;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return NULL;

    int fileSize = fileStat.st_size;
    char *buffer = malloc(fileSize);

    if (buffer == NULL)
    {
        fclose(fp);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, fileSize, fp);
    fclose(fp);

    if (bytesRead != fileSize)
    {
        free(buffer);
        return NULL;
    }

    struct Data *fileData = malloc(sizeof(struct Data));

    if (fileData == NULL)
    {
        free(buffer);
        return NULL;
    }

    fileData->data = buffer;
    fileData->size = fileSize;

    return fileData;
}

void freeFile(struct Data *fileData)
{
    if (fileData != NULL)
    {
        free(fileData->data);
        free(fileData);
    }
}