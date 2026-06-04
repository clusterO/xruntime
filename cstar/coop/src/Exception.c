#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Exception.h"
#include "new.h"
#include "new.r"
#include "Object.h"

extern const struct Class _Object;
extern const struct Class _Class;

static void * Exception_ctor (void * _self, va_list * app) {
    return _self;
}

static const struct Class _Exception = {
    & _Class, & _Object, "Exception", sizeof(struct Exception),
    Exception_ctor, 0, 0, 0, 0, 0, 0, 0, 0
};

const void * Exception = & _Exception;

static struct Exception ** stack;
static int top = 0;
static int capacity = 0;

struct Exception * pushException (void * _self) {
    struct Exception * self = _self;
    if (top == capacity) {
        capacity = capacity ? capacity * 2 : 10;
        stack = realloc(stack, capacity * sizeof(struct Exception *));
    }
    stack[top++] = self;
    return self;
}

void popException (void) {
    assert(top > 0);
    top--;
}

void cause (int number) {
    assert(top > 0);
    longjmp(stack[top-1]->buffer, number);
}
