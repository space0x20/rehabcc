#include "rehabcc.h"

Ast *new_ast(AstKind kind, Type *type)
{
    Ast *ast = calloc(1, sizeof(Ast));
    ast->kind = kind;
    ast->type = type;
    return ast;
}

Ast *new_ast_unary(AstKind kind, Type *type, Ast *lhs)
{
    Ast *ast = new_ast(kind, type);
    ast->lhs = lhs;
    return ast;
}
