#include "rehabcc.h"

struct vector *new_vector(void)
{
    struct vector *vec = calloc(1, sizeof(struct vector));
    vec->data = calloc(16, sizeof(void *));
    vec->size = 0;
    vec->cap = 16;
    return vec;
}

void vector_push_back(struct vector *vec, void *data)
{
    if (vec->size == vec->cap) {
        vec->cap *= 2;
        vec->data = realloc(vec->data, vec->cap);
    }
    vec->data[vec->size++] = data;
}
