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
        Ast *node = new_node(AST_RETURN);
        node->ret = parse_expr();
        expect_token(TK_SCOLON);
        return node;
    }

    if (consume_token(TK_IF)) {
        Ast *node = new_node(AST_IF);
        expect_token(TK_LPAREN);
        node->cond = parse_expr();
        expect_token(TK_RPAREN);
        node->then = parse_stmt();
        node->els = consume_token(TK_ELSE) ? parse_stmt() : NULL;
        return node;
    }

    if (consume_token(TK_WHILE)) {
        Ast *node = new_node(AST_WHILE);
        expect_token(TK_LPAREN);
        node->cond = parse_expr();
        expect_token(TK_RPAREN);
        node->stmt = parse_stmt();
        return node;
    }

    if (consume_token(TK_FOR)) {
        Ast *node = new_node(AST_FOR);
        expect_token(TK_LPAREN);
        if (!consume_token(TK_SCOLON)) {
            node->init = parse_expr();
            expect_token(TK_SCOLON);
        }
        if (!consume_token(TK_SCOLON)) {
            node->cond = parse_expr();
            expect_token(TK_SCOLON);
        }
        if (!consume_token(TK_RPAREN)) {
            node->update = parse_expr();
            expect_token(TK_RPAREN);
        }
        node->stmt = parse_stmt();
        return node;
    }

    if (consume_token(TK_LBRACE)) {
        Ast *node = new_node(AST_BLOCK);
        node->stmts = new_vector();
        while (!consume_token(TK_RBRACE)) {
            push_back(node->stmts, (void *)parse_stmt());
        }
        return node;
    }

    Type *type = parse_type();
    if (type) {
        Ast *node = new_node(AST_VARDECL);
        Token *tok = consume_token(TK_IDENT);
        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            add_lvar(tok, type);
        }
        expect_token(TK_SCOLON);
        return node;
    }

    Ast *node = parse_expr();
    expect_token(TK_SCOLON);
    return node;
}

static Ast *parse_expr(void)
{
    return parse_assign();
}

static Ast *parse_assign(void)
{
    Ast *node = parse_equality();
    if (consume_token(TK_ASSIGN)) {
        node = new_node_binop(AST_ASSIGN, node, parse_assign());
        node->type = node->lhs->type;
    }
    return node;
}

static Ast *parse_equality(void)
{
    Ast *node = parse_relational();

    while (1) {
        if (consume_token(TK_EQ)) {
            node = new_node_binop(AST_EQ, node, parse_relational());
            node->type = int_type();
        }
        else if (consume_token(TK_NE)) {
            node = new_node_binop(AST_NE, node, parse_relational());
            node->type = int_type();
        }
        else {
            return node;
        }
    }
}

static Ast *parse_relational(void)
{
    Ast *node = parse_add();

    while (1) {
        if (consume_token(TK_LT)) {
            node = new_node_binop(AST_LT, node, parse_add());
            node->type = int_type();
        }
        else if (consume_token(TK_LE)) {
            node = new_node_binop(AST_LE, node, parse_add());
            node->type = int_type();
        }
        else if (consume_token(TK_GT)) {
            node = new_node_binop(AST_LT, parse_add(), node);
            node->type = int_type();
        }
        else if (consume_token(TK_GE)) {
            node = new_node_binop(AST_LE, parse_add(), node);
            node->type = int_type();
        }
        else {
            return node;
        }
    }
}

static Ast *parse_add(void)
{
    Ast *node = parse_mul();

    while (1) {
        if (consume_token(TK_PLUS)) {
            if (node->kind == AST_LVAR && node->lvar->type->bt == T_PTR) {
                node = new_node_binop(AST_ADD_PTR, node, parse_mul());
                Type *to = node->lhs->lvar->type->ptr_to;
                node->type_size = to->nbyte;
                node->type = node->lhs->lvar->type;
            }
            else {
                node = new_node_binop(AST_ADD, node, parse_mul());
                node->type = int_type();
            }
        }
        else if (consume_token(TK_MINUS)) {
            node = new_node_binop(AST_SUB, node, parse_mul());
            node->type = int_type();
        }
        else {
            return node;
        }
    }
}

static Ast *parse_mul(void)
{
    Ast *node = parse_unary();

    while (1) {
        if (consume_token(TK_MUL)) {
            node = new_node_binop(AST_MUL, node, parse_unary());
            node->type = int_type();
        }
        else if (consume_token(TK_DIV)) {
            node = new_node_binop(AST_DIV, node, parse_unary());
            node->type = int_type();
        }
        else {
            return node;
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
