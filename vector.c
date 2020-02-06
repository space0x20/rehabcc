#include "rehabcc.h"

Vector *new_vector(void)
{
    Vector *vec = calloc(1, sizeof(Vector));
    vec->data = calloc(16, sizeof(void *));
    vec->size = 0;
    vec->cap = 16;
    return vec;
}

void vector_push_back(Vector *vec, void *data)
{
    if (vec->size == vec->cap) {
        vec->cap *= 2;
        vec->data = realloc(vec->data, vec->cap);
    }
    vec->data[vec->size++] = data;
}
