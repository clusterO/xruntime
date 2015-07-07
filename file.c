#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "file.h"

struct data *loadFile(char *filename) 
{
    char *buffer, *p;
    struct stat buf;
    int readBytes, remainingBytes, totalBytes = 0;

    if(stat(filename, &buf) == -1)
        return NULL;

    if(!(buf.st_mode & S_IFREG)) 
        return NULL;

    FILE *fp = fopen(filename, "rb");

    if(fp == NULL)
        return NULL;

    remainingBytes = buf.st_size;
    p = buffer = malloc(remainingBytes);

    if(buffer == NULL)
        return NULL;

    while(readBytes = fread(p, 1, remainingBytes, fp), readBytes != 0 && remainingBytes > 0) {
        if(readBytes == -1) {
            free(buffer);
            return NULL;
        }

        remainingBytes -= readBytes;
        p += readBytes;
        totalBytes += readBytes;
    }

    struct data *fileData = malloc(sizeof *fileData);

    if(fileData == NULL) {
        free(buffer);
        return NULL;
    }

    fileData->data = buffer;
    fileData->size = totalBytes;

    return fileData;
}

void freeFile(struct data *fileData) 
{
    free(fileData->data);
    free(fileData);
}

