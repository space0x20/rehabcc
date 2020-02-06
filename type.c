#include "rehabcc.h"

static Type *new_type(BasicType bt)
{
    Type *type = calloc(1, sizeof(Type));
    type->bt = bt;
    return type;
}

Type *int_type(void)
{
    static Type *type = NULL;
    if (!type) {
        type = new_type(T_INT);
        type->nbyte = 4;
    }
    return type;
}

Type *ptr_type(Type *ptr_to)
{
    Type *type = new_type(T_PTR);
    type->ptr_to = ptr_to;
    type->nbyte = 8;
    return type;
}

Type *deref_type(Type *type)
{
    return type->ptr_to;
}
