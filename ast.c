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

Ast *new_ast_binary(AstKind kind, Type *type, Ast *lhs, Ast *rhs)
{
    Ast *ast = new_ast(kind, type);
    ast->lhs = lhs;
    ast->rhs = rhs;
    return ast;
}

Ast *new_ast_num(int val)
{
    Ast *ast = new_ast(AST_NUM, int_type());
    ast->val = val;
    return ast;
}
