#include "rehabcc.h"

static struct type *new_type(enum basic_type bt)
{
    struct type *type = calloc(1, sizeof(struct type));
    type->bt = bt;
    return type;
}

struct type *void_type(void)
{
    static struct type *type = NULL;
    if (!type) {
        type = new_type(T_VOID);
        type->nbyte = 0;
    }
    return type;
}

struct type *int_type(void)
{
    static struct type *type = NULL;
    if (!type) {
        type = new_type(T_INT);
        type->nbyte = 4;
    }
    return type;
}

struct type *ptr_type(struct type *ptr_to)
{
    struct type *type = new_type(T_PTR);
    type->ptr_to = ptr_to;
    type->nbyte = 8;
    return type;
}

struct type *deref_type(struct type *type)
{
    return type->ptr_to;
}

struct type *array_type(struct type *array_of, int size)
{
    struct type *type = new_type(T_ARRAY);
    type->ptr_to = array_of;
    type->array_size = size;
    type->nbyte = array_of->nbyte * size;
    return type;
}
