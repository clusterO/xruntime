#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <setjmp.h>

extern const void * Exception;

struct Exception {
    const void * class;
    jmp_buf buffer;
};

void cause (int number);
struct Exception * pushException (void * self);
void popException (void);

#define catch(e) if (setjmp(pushException(e)->buffer) == 0)

#endif
