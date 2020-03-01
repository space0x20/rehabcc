#include "rehabcc.h"

// 文法
// program    = (function | global_var)*
// function   = type ident "(" paramlist? ")" block
// global_var = type ident ("[" num "]")? ";"
// block      = "{" stmt* "}"
// stmt       = expr ";"
//            | block
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//            | "return" expr ";"
//            | type ident ("[" num "]")*;
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = primary ("*" primary | "/" primary)*
// unary      = "+"? primary
//            | "-"? primary
//            | "*" unary
//            | "&" unary
//            | "sizeof" unary
// primary    = num
//            | ident "(" arglist? ")" ... 関数呼び出し
//            | ident ("[" expr "]")*  ... 変数
//            | "(" expr ")"
//
// arglist    = expr ("," expr)*
// paramlist  = type ident ("," type ident)*
// type       = "int" "*"*

static void parse_program(void);
static struct ast *parse_function(void);
static void parse_global_var(void);
static struct ast *parse_stmt(void);
static struct ast *parse_expr(void);
static struct ast *parse_assign(void);
static struct ast *parse_equality(void);
static struct ast *parse_relational(void);
static struct ast *parse_add(void);
static struct ast *parse_mul(void);
static struct ast *parse_unary(void);
static struct ast *parse_primary(void);

static struct vector *parse_arglist(void);
static struct type *parse_type(void);
static struct type *parse_type_postfix(struct type *);

void parse(void)
{
    parse_program();
}

static void parse_program(void)
{
    int i = 0;
    while (!consume_token(TK_EOF)) {
        struct token *backup = get_token();
        struct type *type = parse_type();
        if (!type) {
            error_token("関数の返り値の型あるいはグローバル変数の型が与えられていません");
        }
        expect_token(TK_IDENT);
        if (consume_token(TK_LPAREN)) {
            set_token(backup);
            add_ast(parse_function());
        }
        else {
            set_token(backup);
            parse_global_var();
        }
    }
}

static struct ast *parse_function(void)
{
    struct ast *ast;
    struct token *tok;
    struct type *type;

    // return type
    type = parse_type();
    if (!type) {
        error_token("関数の返り値の型が宣言されていません");
    }
    ast = new_ast(AST_FUNCTION, type);

    // function name
    tok = consume_token(TK_IDENT);
    ast->funcname = copy_token_str(tok);

    // parameter
    ast->params = new_vector();
    clear_local_vars();
    expect_token(TK_LPAREN);
    while (!consume_token(TK_RPAREN)) {
        type = parse_type();
        if (!type) {
            error_token("仮引数の型が宣言されていません");
        }
        tok = consume_token(TK_IDENT);
        if (!tok) {
            error_token("不正な引数の名前です");
        }
        vector_push_back(ast->params, copy_token_str(tok));
        struct var *lvar = find_local_var(tok);
        if (!lvar) {
            add_local_var(tok, type);
        }
        if (!consume_token(TK_COLON)) {
            expect_token(TK_RPAREN);
            break;
        }
    }

    // 本体
    ast->stmts = new_vector();
    expect_token(TK_LBRACE);
    while (!consume_token(TK_RBRACE)) {
        vector_push_back(ast->stmts, parse_stmt());
    }

    ast->locals = get_local_vars();
    return ast;
}

static void parse_global_var(void)
{
    struct type *type = parse_type();
    struct token *ident = expect_token(TK_IDENT);
    type = parse_type_postfix(type);
    // todo: グローバル変数の初期値
    expect_token(TK_SCOLON);
    if (find_global_var(ident)) {
        error_token("すでに定義されているグローバル変数です");
    }
    add_global_var(ident, type);
}

static struct ast *parse_stmt(void)
{
    if (consume_token(TK_RETURN)) {
        struct ast *lhs = parse_expr();
        expect_token(TK_SCOLON);
        return new_ast_unary(AST_RETURN, lhs->type, lhs);
    }

    if (consume_token(TK_IF)) {
        struct ast *ast = new_ast(AST_IF, NULL);
        expect_token(TK_LPAREN);
        ast->cond = parse_expr();
        expect_token(TK_RPAREN);
        ast->then = parse_stmt();
        ast->els = consume_token(TK_ELSE) ? parse_stmt() : NULL;
        return ast;
    }

    if (consume_token(TK_WHILE)) {
        struct ast *ast = new_ast(AST_WHILE, NULL);
        expect_token(TK_LPAREN);
        ast->cond = parse_expr();
        expect_token(TK_RPAREN);
        ast->stmt = parse_stmt();
        return ast;
    }

    if (consume_token(TK_FOR)) {
        struct ast *ast = new_ast(AST_FOR, NULL);
        expect_token(TK_LPAREN);
        if (!consume_token(TK_SCOLON)) {
            ast->init = parse_expr();
            expect_token(TK_SCOLON);
        }
        if (!consume_token(TK_SCOLON)) {
            ast->cond = parse_expr();
            expect_token(TK_SCOLON);
        }
        if (!consume_token(TK_RPAREN)) {
            ast->update = parse_expr();
            expect_token(TK_RPAREN);
        }
        ast->stmt = parse_stmt();
        return ast;
    }

    if (consume_token(TK_LBRACE)) {
        struct ast *ast = new_ast(AST_BLOCK, NULL);
        ast->stmts = new_vector();
        while (!consume_token(TK_RBRACE)) {
            vector_push_back(ast->stmts, (void *)parse_stmt());
        }
        return ast;
    }

    struct type *type = parse_type();
    if (type) {
        struct ast *ast = new_ast(AST_VARDECL, NULL);
        struct token *tok = consume_token(TK_IDENT);
        type = parse_type_postfix(type);
        struct var *lvar = find_local_var(tok);
        if (lvar) {
            error_token("変数を重複して宣言しています");
        }
        add_local_var(tok, type);
        expect_token(TK_SCOLON);
        return ast;
    }

    struct ast *ast = parse_expr();
    expect_token(TK_SCOLON);
    return ast;
}

static struct ast *parse_expr(void)
{
    return parse_assign();
}

static struct ast *parse_assign(void)
{
    struct ast *ast = parse_equality();
    if (consume_token(TK_ASSIGN)) {
        struct ast *rhs = parse_assign();
        ast = new_ast_binary(AST_ASSIGN, rhs->type, ast, rhs);
    }
    return ast;
}

static struct ast *parse_equality(void)
{
    struct ast *ast = parse_relational();
    while (1) {
        if (consume_token(TK_EQ)) {
            ast = new_ast_binary(AST_EQ, int_type(), ast, parse_relational());
        }
        else if (consume_token(TK_NE)) {
            ast = new_ast_binary(AST_NE, int_type(), ast, parse_relational());
        }
        else {
            return ast;
        }
    }
}

static struct ast *parse_relational(void)
{
    struct ast *ast = parse_add();
    while (1) {
        if (consume_token(TK_LT)) {
            ast = new_ast_binary(AST_LT, int_type(), ast, parse_add());
        }
        else if (consume_token(TK_LE)) {
            ast = new_ast_binary(AST_LE, int_type(), ast, parse_add());
        }
        else if (consume_token(TK_GT)) {
            ast = new_ast_binary(AST_LT, int_type(), parse_add(), ast);
        }
        else if (consume_token(TK_GE)) {
            ast = new_ast_binary(AST_LE, int_type(), parse_add(), ast);
        }
        else {
            return ast;
        }
    }
}

static struct ast *parse_add(void)
{
    struct ast *ast = parse_mul();
    while (1) {
        if (consume_token(TK_PLUS)) {
            // ポインタと整数の足し算
            // todo: ポインタが左辺に来る場合しか取り扱っていない
            if (ast->kind == AST_LVAR && (ast->var->type->bt == T_PTR || ast->var->type->bt == T_ARRAY)) {
                ast = new_ast_binary(AST_ADD_PTR, ast->var->type, ast, parse_mul());
            }
            else {
                ast = new_ast_binary(AST_ADD, int_type(), ast, parse_mul());
            }
        }
        else if (consume_token(TK_MINUS)) {
            ast = new_ast_binary(AST_SUB, int_type(), ast, parse_mul());
        }
        else {
            return ast;
        }
    }
}

static struct ast *parse_mul(void)
{
    struct ast *ast = parse_unary();
    while (1) {
        if (consume_token(TK_MUL)) {
            ast = new_ast_binary(AST_MUL, int_type(), ast, parse_unary());
        }
        else if (consume_token(TK_DIV)) {
            ast = new_ast_binary(AST_DIV, int_type(), ast, parse_unary());
        }
        else {
            return ast;
        }
    }
}

static struct ast *parse_unary(void)
{
    struct ast *lhs;
    if (consume_token(TK_MUL)) {
        lhs = parse_unary();
        return new_ast_unary(AST_DEREF, deref_type(lhs->type), lhs);
    }
    if (consume_token(TK_AND)) {
        lhs = parse_unary();
        return new_ast_unary(AST_ADDR, ptr_type(lhs->type), lhs);
    }
    if (consume_token(TK_SIZEOF)) {
        lhs = parse_unary();
        return new_ast_num(lhs->type->nbyte);
    }
    if (consume_token(TK_PLUS)) {
        return parse_primary();
    }
    if (consume_token(TK_MINUS)) {
        return new_ast_binary(AST_SUB, int_type(), new_ast_num(0), parse_primary());
    }
    return parse_primary();
}

static struct ast *parse_primary(void)
{
    struct ast *ast;
    struct token *tok;

    if (consume_token(TK_LPAREN)) {
        ast = parse_expr();
        expect_token(TK_RPAREN);
        return ast;
    }

    tok = consume_token(TK_IDENT);
    if (tok) {
        if (consume_token(TK_LPAREN)) {
            ast = new_ast(AST_FUNCALL, NULL); // Todo : NULL の代わりに関数の戻り値型を指定する
            ast->funcname = copy_token_str(tok);
            if (consume_token(TK_RPAREN)) {
                ast->params = new_vector();
            }
            else {
                ast->params = parse_arglist();
                expect_token(TK_RPAREN);
            }
            return ast;
        }

        struct var *var = find_local_var(tok);
        if (var) {
            ast = new_ast(AST_LVAR, var->type);
            ast->var = var;
            if (consume_token(TK_LBRACKET)) {
                struct ast *add = new_ast_binary(AST_ADD_PTR, int_type(), ast, parse_expr());
                ast = new_ast_unary(AST_DEREF, deref_type(var->type), add);
                expect_token(TK_RBRACKET);
            }
            return ast;
        }
        var = find_global_var(tok);
        if (var) {
            ast = new_ast(AST_GVAR, var->type);
            ast->var = var;
            if (consume_token(TK_LBRACKET)) {
                struct ast *add = new_ast_binary(AST_ADD_PTR, int_type(), ast, parse_expr());
                ast = new_ast_unary(AST_DEREF, deref_type(var->type), add);
                expect_token(TK_RBRACKET);
            }
            return ast;
        }
        error_token("定義されていない変数を使用しています");
    }

    tok = consume_token(TK_NUM);
    if (tok) {
        ast = new_ast(AST_NUM, int_type());
        ast->val = tok->val;
        return ast;
    }

    error_token("パーズできません");
}

static struct vector *parse_arglist(void)
{
    struct vector *args = new_vector();
    vector_push_back(args, parse_expr());
    while (consume_token(TK_COLON)) {
        vector_push_back(args, parse_expr());
    }
    return args;
}

static struct type *parse_type(void)
{
    if (!consume_token(TK_INT)) {
        return NULL;
    }
    struct type *type = int_type();
    while (consume_token(TK_MUL)) {
        type = ptr_type(type);
    }
    return type;
}

static struct type *parse_type_postfix(struct type *type)
{
    while (consume_token(TK_LBRACKET)) {
        struct token *num = expect_token(TK_NUM); // todo: 配列数は定数のみ可
        type = array_type(type, num->val);
        expect_token(TK_RBRACKET);
    }
    return type;
}
