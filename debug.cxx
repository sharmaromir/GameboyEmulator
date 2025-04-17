#include "debug.h"
#include <stdlib.h>
#include <stdio.h>

void missing(char const * const file, int const line) {
    printf("missing code at %s:%d\n",file,line);
    exit(-1);
}
