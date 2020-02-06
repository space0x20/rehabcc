#include "rehabcc.h"

static Type *new_type(BasicType bt)
{
    Type *type = calloc(1, sizeof(Type));
    type->type = bt;
    return type;
}

Type *type_int(void)
{
    static Type *type = NULL;
    if (!type) {
        type = new_type(T_INT);
    }
    return type;
}

Type *type_ptr(Type *ptr_to)
{
    Type *type = new_type(T_PTR);
    type->ptr_to = ptr_to;
    return type;
}

Type *type_deref(Type *type)
{
    return type->ptr_to;
}
