#include "rehabcc.h"

struct ast *new_ast(enum ast_kind kind, struct type *type)
{
    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->kind = kind;
    ast->type = type;
    return ast;
}

struct ast *new_ast_unary(enum ast_kind kind, struct type *type, struct ast *lhs)
{
    struct ast *ast = new_ast(kind, type);
    ast->lhs = lhs;
    return ast;
}

struct ast *new_ast_binary(enum ast_kind kind, struct type *type, struct ast *lhs, struct ast *rhs)
{
    struct ast *ast = new_ast(kind, type);
    ast->lhs = lhs;
    ast->rhs = rhs;
    return ast;
}

struct ast *new_ast_num(int val)
{
    struct ast *ast = new_ast(AST_NUM, int_type());
    ast->val = val;
    return ast;
}
