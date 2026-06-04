#include "Point.h"
#include "Point.r"
#include "new.h"
#include "Object.h"

static struct Class _Point = {
    & _Class, Object(), sizeof(struct Point),
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

const void * Point(void) {
    static const void * _Point_ptr = 0;
    if (!_Point_ptr) _Point_ptr = &_Point;
    return _Point_ptr;
}
