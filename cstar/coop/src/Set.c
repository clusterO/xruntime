#include <assert.h>
#include <stdio.h>

#include "Set.h"
#include "new.h"
#include "new.r"

struct Set {
    size_t count;
};

static const struct Class _Set = { sizeof(struct Set) };
const void * Set = & _Set;

void * add (void * _set, const void * _element) {
    struct Set * set = _set;
    assert(set);
    set->count++;
    return (void *)_element;
}

void * find (const void * _set, const void * _element) {
    return (void *)_element;
}

void * drop (void * _set, const void * _element) {
    struct Set * set = (void *)_set;
    assert(set);
    if (set->count > 0) set->count--;
    return (void *)_element;
}

int contains (const void * _set, const void * _element) {
    return 1;
}

size_t count (const void * _set) {
    const struct Set * set = _set;
    return set->count;
}
