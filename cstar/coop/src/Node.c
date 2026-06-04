#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include "Node.h"
#include "new.h"
#include "new.r"

/* Value */
struct Value {
    const void * class;
    double value;
};

static void * Value_ctor (void * _self, va_list * app) {
    struct Value * self = _self;
    self->value = va_arg(* app, double);
    return self;
}

static double Value_exec (const void * _self) {
    const struct Value * self = _self;
    return self->value;
}

static const struct Class _Value = {
    sizeof(struct Value),
    Value_ctor, 0, 0, 0,
    Value_exec
};

const void * Value = & _Value;

/* Binary Nodes */
struct Bin {
    const void * class;
    void * left, * right;
};

static void * Bin_ctor (void * _self, va_list * app) {
    struct Bin * self = _self;
    self->left = va_arg(* app, void *);
    self->right = va_arg(* app, void *);
    return self;
}

static void * Bin_dtor (void * _self) {
    struct Bin * self = _self;
    delete(self->left);
    delete(self->right);
    return self;
}

/* Add */
static double Add_exec (const void * _self) {
    const struct Bin * self = _self;
    return exec(self->left) + exec(self->right);
}

static const struct Class _Add = {
    sizeof(struct Bin),
    Bin_ctor, Bin_dtor, 0, 0,
    Add_exec
};

const void * Add = & _Add;

/* Sub */
static double Sub_exec (const void * _self) {
    const struct Bin * self = _self;
    return exec(self->left) - exec(self->right);
}

static const struct Class _Sub = {
    sizeof(struct Bin),
    Bin_ctor, Bin_dtor, 0, 0,
    Sub_exec
};

const void * Sub = & _Sub;
