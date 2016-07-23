#ifndef _LOCAL_FILE_HEADER_
#define _LOCAL_FILE_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

struct Data
{
    int size;
    void *data;
};

extern struct Data *loadFile(char *fileName);
extern void freeFile(struct Data *fileData);

#endif