#include <stdarg.h>
#include <stdio.h>

#include "Circle.h"
#include "Circle.r"
#include "new.h"
#include "new.r"
#include "Object.h"

static void * Circle_ctor (void * _self, va_list * app) {
    struct Circle * self = _self;
    self->point.x = va_arg(* app, int);
    self->point.y = va_arg(* app, int);
    self->rad = va_arg(* app, int);
    return self;
}

const void * Circle (void) {
    static const void * _Circle = 0;
    if (! _Circle) {
        _Circle = new(Class, "Circle", Point(), sizeof(struct Circle), 
                      "ctor", Circle_ctor, 
                      NULL);
    }
    return _Circle;
}
