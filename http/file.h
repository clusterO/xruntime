#ifndef _FILEL_HEADER_
#define _FILEL_HEADER_

struct Data {
    int size;
    void *data;
};

extern struct Data *loadFile(char *fileName);
extern void freeFile(struct Data *fileData);

#endif