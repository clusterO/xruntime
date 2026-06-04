#include <stdio.h>
#include "new.h"
#include "Exception.h"

int main () {
    void * e = new(Exception);

    catch(e) {
        printf("Trying something risky...\n");
        cause(1);
        printf("This should not be printed\n");
    } else {
        printf("Caught exception!\n");
    }

    popException();
    delete(e);
    printf("Done.\n");
    return 0;
}
