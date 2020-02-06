#include "rehabcc.h"

Ast *new_ast(AstKind kind, Type *type)
{
    Ast *ast = calloc(1, sizeof(Ast));
    ast->kind = kind;
    ast->type = type;
    return ast;
}

