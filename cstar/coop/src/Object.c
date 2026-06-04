#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Object.h"
#include "Object.r"
#include "new.h"
#include "new.r"

extern const struct Class _Object;
extern const struct Class _Class;

/* Object methods */
static void * Object_ctor (void * self, va_list * app) {
    return self;
}

static void * Object_dtor (void * self) {
    return self;
}

static int Object_differ (const void * self, const void * b) {
    return self != b;
}

static int Object_puto (const void * self, FILE * fp) {
    const struct Class * const * cp = self;
    return fprintf(fp, "%s at %p\n", (* cp)->name, self);
}

/* Class methods */
static void * Class_ctor (void * _self, va_list * app) {
    struct Class * self = _self;
    const char * tag;
    
    self->name = va_arg(* app, const char *);
    self->super = va_arg(* app, const void *);
    self->size = va_arg(* app, size_t);

    if (self->super) {
        size_t offset = offsetof(struct Class, ctor);
        memcpy((char *)self + offset, (char *)self->super + offset,
               sizeof(struct Class) - offset);
    }

    while ((tag = va_arg(* app, const char *))) {
        void * method = va_arg(* app, void *);
        if (strcmp(tag, "ctor") == 0) self->ctor = method;
        else if (strcmp(tag, "dtor") == 0) self->dtor = method;
        else if (strcmp(tag, "exec") == 0) self->exec = method;
        else if (strcmp(tag, "puto") == 0) self->puto = method;
    }

    return self;
}

static void * Class_dtor (void * _self) {
    return 0;
}

static void Class_reclaim (const void * class) {
    printf("Reclaiming instances of class\n");
}

/* Bootstrap */
const struct Class _Object = {
    & _Class, 0, "Object", sizeof(struct Object),
    Object_ctor, Object_dtor, 0, Object_differ, 0, 0, 0, Object_puto, 0
};

const struct Class _Class = {
    & _Class, & _Object, "Class", sizeof(struct Class),
    Class_ctor, Class_dtor, 0, Object_differ, 0, 0, Class_reclaim, 0, 0
};

const void * Object = & _Object;
const void * Class = & _Class;

const void * super (const void * _self) {
    const struct Class * self = _self;
    assert(self && self->super);
    return self->super;
}

size_t sizeOf (const void * _self) {
    const struct Class * const * cp = _self;
    assert(_self && * cp);
    return (* cp)->size;
}

int isA (const void * _self, const void * _class) {
    if (_self) {
        const struct Class * const * cp = _self;
        return * cp == _class;
    }
    return 0;
}

int isOf (const void * _self, const void * _class) {
    if (_self) {
        const struct Class * cp = * (const struct Class **) _self;
        while (cp) {
            if (cp == _class) return 1;
            cp = cp->super;
        }
    }
    return 0;
}

void * cast (const void * _self, const void * _class) {
    assert(isOf(_self, _class));
    return (void *) _self;
}

int respondsTo (const void * _self, const char * tag) {
    if (_self && tag) {
        const struct Class * const * cp = _self;
        if ((* cp)->respondsTo) {
            return (* cp)->respondsTo(_self, tag);
        }
        if (strcmp(tag, "ctor") == 0) return (* cp)->ctor != 0;
        if (strcmp(tag, "dtor") == 0) return (* cp)->dtor != 0;
        if (strcmp(tag, "exec") == 0) return (* cp)->exec != 0;
    }
    return 0;
}

void reclaim (const void * _class) {
    const struct Class * const * cp = _class;
    assert(_class && * cp && (* cp)->reclaim);
    (* cp)->reclaim(_class);
}

int puto (const void * _self, FILE * fp) {
    const struct Class * const * cp = _self;
    assert(_self && * cp && (* cp)->puto);
    return (* cp)->puto(_self, fp);
}

void * geto (void * _self, FILE * fp) {
    const struct Class * const * cp = _self;
    assert(_self && * cp && (* cp)->geto);
    return (* cp)->geto(_self, fp);
}
