#ifndef NEW_R
#define NEW_R

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

struct Class {
    const struct Class * class; // Metaclass
    const struct Class * super; // Superclass
    const char * name; // Class name for serialization
    size_t size;
    void * (* ctor) (void * self, va_list * app);
    void * (* dtor) (void * self);
    void * (* clone) (const void * self);
    int (* differ) (const void * self, const void * b);
    double (* exec) (const void * self);
    int (* respondsTo) (const void * self, const char * tag);
    void (* reclaim) (const void * self);
    int (* puto) (const void * self, FILE * fp);
    void * (* geto) (void * self, FILE * fp);
};

#endif
