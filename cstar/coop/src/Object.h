#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>

#include <stdio.h>

extern const void * Object;
extern const void * Class;

const void * super (const void * self);
size_t sizeOf (const void * self);
int isA (const void * self, const void * class);
int isOf (const void * self, const void * class);
void * cast (const void * self, const void * class);
int respondsTo (const void * self, const char * tag);
void reclaim (const void * class);

int puto (const void * self, FILE * fp);
void * geto (void * self, FILE * fp);
void * retrieve (FILE * fp);

#endif
