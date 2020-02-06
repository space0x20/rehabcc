#include <stdlib.h>

int *alloc4(int x1, int x2, int x3, int x4)
{
    int *x = calloc(4, sizeof(int));
    x[0] = x1;
    x[1] = x2;
    x[2] = x3;
    x[3] = x4;
    return x;
}
