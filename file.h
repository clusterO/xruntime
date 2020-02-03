#ifndef _FILEL_HEADER_
#define _FILEL_HEADER_

struct data {
    int size;
    void *data;
};

extern struct data *loadFile(char *filename);
extern void freeFile(struct data *data);

#endif