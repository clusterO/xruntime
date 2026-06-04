#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "Name.h"
#include "Name.r"
#include "new.h"
#include "new.r"

static void * Name_ctor (void * _self, va_list * app) {
    struct Name * self = _self;
    const char * name = va_arg(* app, const char *);
    self->name = strdup(name);
    return self;
}

static void * Name_dtor (void * _self) {
    struct Name * self = _self;
    free((void *)self->name);
    return self;
}

const char * getName (const void * _self) {
    const struct Name * self = _self;
    return self->name;
}

static const struct Class _Name = {
    sizeof(struct Name),
    Name_ctor, Name_dtor, 0, 0, 0
};

const void * Name = & _Name;
