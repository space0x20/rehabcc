#include "rehabcc.h"

static void init_lvar(void)
{
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = NULL;
    lvar->name = NULL;
    lvar->len = 0;
    lvar->offset = 0;
    locals = lvar;
}

// ローカル変数を追加する
static LVar *add_lvar(Token *tok, Type *type)
{
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = locals->offset + 8;
    lvar->type = type;
    locals = lvar;
    return lvar;
}

// ローカル変数を検索する
// 見つかれば LVar へのポインタ、見つからなければ NULL
static LVar *find_lvar(Token *tok)
{
    for (LVar *lvar = locals; lvar != NULL; lvar = lvar->next) {
        if (lvar->len == tok->len && !memcmp(lvar->name, tok->str, lvar->len)) {
            return lvar;
        }
    }
    return NULL;
}

static Ast *new_node(AstKind kind)
{
    Ast *node = calloc(1, sizeof(Ast));
    node->kind = kind;
    return node;
}

static Ast *new_node_binop(AstKind kind, Ast *lhs, Ast *rhs)
{
    Ast *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Ast *new_node_num(int val)
{
    Ast *node = new_node(AST_NUM);
    node->val = val;
    node->type = int_type();
    return node;
}

// 文法
// program    = funcdef*
// function   = type ident "(" paramlist? ")" block
// block      = "{" stmt* "}"
// stmt       = expr ";"
//            | block
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//            | "return" expr ";"
//            | type ident;
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
//            | ident                  ... 変数
//            | "(" expr ")"
// arglist    = expr ("," expr)*
// paramlist  = type ident ("," type ident)*
// type       = "int" "*"*

static void parse_program(void);
static Ast *parse_function(void);
static Ast *parse_stmt(void);
static Ast *parse_expr(void);
static Ast *parse_assign(void);
static Ast *parse_equality(void);
static Ast *parse_relational(void);
static Ast *parse_add(void);
static Ast *parse_mul(void);
static Ast *parse_unary(void);
static Ast *parse_primary(void);

static Vector *parse_arglist(void);
static Type *parse_type(void);

void parse(void)
{
    parse_program();
}

static void parse_program(void)
{
    int i = 0;
    while (!consume_token(TK_EOF)) {
        code[i++] = parse_function();
    }
    code[i] = NULL;
}

static Ast *parse_function(void)
{
    Ast *ast;
    Token *tok;
    Type *type;

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
    init_lvar();
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
        push_back(ast->params, copy_token_str(tok));
        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            add_lvar(tok, type);
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
        push_back(ast->stmts, parse_stmt());
    }

    ast->locals = locals;
    return ast;
}

static Ast *parse_stmt(void)
{
    if (consume_token(TK_RETURN)) {
        Ast *lhs = parse_expr();
        expect_token(TK_SCOLON);
        return new_ast_unary(AST_RETURN, lhs->type, lhs);
    }

    if (consume_token(TK_IF)) {
        Ast *ast = new_ast(AST_IF, NULL);
        expect_token(TK_LPAREN);
        ast->cond = parse_expr();
        expect_token(TK_RPAREN);
        ast->then = parse_stmt();
        ast->els = consume_token(TK_ELSE) ? parse_stmt() : NULL;
        return ast;
    }

    if (consume_token(TK_WHILE)) {
        Ast *ast = new_ast(AST_WHILE, NULL);
        expect_token(TK_LPAREN);
        ast->cond = parse_expr();
        expect_token(TK_RPAREN);
        ast->stmt = parse_stmt();
        return ast;
    }

    if (consume_token(TK_FOR)) {
        Ast *ast = new_ast(AST_FOR, NULL);
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
        Ast *ast = new_ast(AST_BLOCK, NULL);
        ast->stmts = new_vector();
        while (!consume_token(TK_RBRACE)) {
            push_back(ast->stmts, (void *)parse_stmt());
        }
        return ast;
    }

    Type *type = parse_type();
    if (type) {
        Ast *ast = new_ast(AST_VARDECL, NULL);
        Token *tok = consume_token(TK_IDENT);
        LVar *lvar = find_lvar(tok);
        if (lvar) {
            error_token("変数を重複して宣言しています");
        }
        add_lvar(tok, type);
        expect_token(TK_SCOLON);
        return ast;
    }

    Ast *ast = parse_expr();
    expect_token(TK_SCOLON);
    return ast;
}

static Ast *parse_expr(void)
{
    return parse_assign();
}

static Ast *parse_assign(void)
{
    Ast *ast = parse_equality();
    if (consume_token(TK_ASSIGN)) {
        Ast *rhs = parse_assign();
        ast = new_ast_binary(AST_ASSIGN, rhs->type, ast, rhs);
    }
    return ast;
}

static Ast *parse_equality(void)
{
    Ast *ast = parse_relational();
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

static Ast *parse_relational(void)
{
    Ast *ast = parse_add();
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

static Ast *parse_add(void)
{
    Ast *ast = parse_mul();
    while (1) {
        if (consume_token(TK_PLUS)) {
            // ポインタと整数の足し算
            // todo: ポインタが左辺に来る場合しか取り扱っていない
            if (ast->kind == AST_LVAR && ast->lvar->type->bt == T_PTR) {
                ast = new_ast_binary(AST_ADD_PTR, ast->lvar->type, ast, parse_mul());
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

static Ast *parse_mul(void)
{
    Ast *ast = parse_unary();
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

static Ast *parse_unary(void)
{
    Ast *lhs;
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

static Ast *parse_primary(void)
{
    Ast *ast;
    Token *tok;

    if (consume_token(TK_LPAREN)) {
        ast = parse_expr();
        expect_token(TK_RPAREN);
        return ast;
    }

    tok = consume_token(TK_IDENT);
    if (tok) {
        if (consume_token(TK_LPAREN)) {
            ast = new_ast(AST_FUNCALL, NULL); // Todo : NULL の代わりに関数の戻り値型を指定する
            ast->func = copy_token_str(tok);
            if (consume_token(TK_RPAREN)) {
                ast->args = new_vector();
            }
            else {
                ast->args = parse_arglist();
                expect_token(TK_RPAREN);
            }
            return ast;
        }

        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            error_token("定義されていない変数を使用しています");
        }
        ast = new_ast(AST_LVAR, lvar->type);
        ast->lvar = lvar;
        return ast;
    }

    tok = consume_token(TK_NUM);
    if (tok) {
        ast = new_ast(AST_NUM, int_type());
        ast->val = tok->val;
        return ast;
    }

    error_token("パーズできません");
}

static Vector *parse_arglist(void)
{
    Vector *args = new_vector();
    push_back(args, parse_expr());
    while (consume_token(TK_COLON)) {
        push_back(args, parse_expr());
    }
    return args;
}

static Type *parse_type(void)
{
    if (!consume_token(TK_INT)) {
        return NULL;
    }
    Type *type = int_type();
    while (consume_token(TK_MUL)) {
        type = ptr_type(type);
    }
    return type;
}
