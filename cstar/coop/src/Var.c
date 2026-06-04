#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "Var.h"
#include "Var.r"
#include "new.h"
#include "new.r"

static void * Var_ctor (void * _self, va_list * app) {
    struct Var * self = _self;
    const char * name = va_arg(* app, const char *);
    self->name.name = strdup(name);
    self->value = va_arg(* app, double);
    return self;
}

static void * Var_dtor (void * _self) {
    struct Var * self = _self;
    free((void *)self->name.name);
    return self;
}

double getValue (const void * _self) {
    const struct Var * self = _self;
    return self->value;
}

static const struct Class _Var = {
    sizeof(struct Var),
    Var_ctor, Var_dtor, 0, 0, 0
};

const void * Var = & _Var;
