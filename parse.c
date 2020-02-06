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

static Type *type_int(void)
{
    static Type *t = NULL;
    if (!t) {
        t = calloc(1, sizeof(Type));
        t->type = T_INT;
    }
    return t;
}

static Type *type_ptr(Type *ptr_to)
{
    Type *t = calloc(1, sizeof(Type));
    t->type = T_PTR;
    t->ptr_to = ptr_to;
    return t;
}

static Type *type_deref(Type *t)
{
    return t->ptr_to;
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
    Ast *node = new_node(ND_NUM);
    node->val = val;
    node->type = type_int();
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
    Ast *node = new_node(ND_FUNCTION);

    node->type = parse_type();
    if (!node->type) {
        error("関数の返り値型が宣言されていません");
    }

    // 関数名
    Token *tok = consume_token(TK_IDENT);
    node->funcname = calloc(tok->len + 1, sizeof(char));
    strncpy(node->funcname, tok->str, tok->len);

    // 引数
    node->params = new_vector();
    init_lvar();
    expect_token(TK_LPAREN);
    while (!consume_token(TK_RPAREN)) {
        Type *type = parse_type();
        if (!type) {
            error("仮引数の型が宣言されていません");
        }

        Token *tok = consume_token(TK_IDENT);
        char *name = copy_str(tok);
        push_back(node->params, name);

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
    node->stmts = new_vector();
    expect_token(TK_LBRACE);
    while (!consume_token(TK_RBRACE)) {
        push_back(node->stmts, parse_stmt());
    }

    node->locals = locals;
    return node;
}

static Ast *parse_stmt(void)
{
    if (consume_token(TK_RETURN)) {
        Ast *node = new_node(ND_RETURN);
        node->ret = parse_expr();
        expect_token(TK_SCOLON);
        return node;
    }

    if (consume_token(TK_IF)) {
        Ast *node = new_node(ND_IF);
        expect_token(TK_LPAREN);
        node->cond = parse_expr();
        expect_token(TK_RPAREN);
        node->then = parse_stmt();
        node->els = consume_token(TK_ELSE) ? parse_stmt() : NULL;
        return node;
    }

    if (consume_token(TK_WHILE)) {
        Ast *node = new_node(ND_WHILE);
        expect_token(TK_LPAREN);
        node->cond = parse_expr();
        expect_token(TK_RPAREN);
        node->stmt = parse_stmt();
        return node;
    }

    if (consume_token(TK_FOR)) {
        Ast *node = new_node(ND_FOR);
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
        Ast *node = new_node(ND_BLOCK);
        node->stmts = new_vector();
        while (!consume_token(TK_RBRACE)) {
            push_back(node->stmts, (void *)parse_stmt());
        }
        return node;
    }

    Type *type = parse_type();
    if (type) {
        Ast *node = new_node(ND_VARDECL);
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
        node = new_node_binop(ND_ASSIGN, node, parse_assign());
        node->type = node->lhs->type;
    }
    return node;
}

static Ast *parse_equality(void)
{
    Ast *node = parse_relational();

    while (1) {
        if (consume_token(TK_EQ)) {
            node = new_node_binop(ND_EQ, node, parse_relational());
            node->type = type_int();
        }
        else if (consume_token(TK_NE)) {
            node = new_node_binop(ND_NE, node, parse_relational());
            node->type = type_int();
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
            node = new_node_binop(ND_LT, node, parse_add());
            node->type = type_int();
        }
        else if (consume_token(TK_LE)) {
            node = new_node_binop(ND_LE, node, parse_add());
            node->type = type_int();
        }
        else if (consume_token(TK_GT)) {
            node = new_node_binop(ND_LT, parse_add(), node);
            node->type = type_int();
        }
        else if (consume_token(TK_GE)) {
            node = new_node_binop(ND_LE, parse_add(), node);
            node->type = type_int();
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
            if (node->kind == ND_LVAR && node->lvar->type->type == T_PTR) {
                node = new_node_binop(ND_ADD_PTR, node, parse_mul());
                Type *to = node->lhs->lvar->type->ptr_to;
                if (to->type == T_PTR) {
                    node->type_size = 8;
                }
                else if (to->type == T_INT) {
                    node->type_size = 4;
                }
                node->type = node->lhs->lvar->type;
            }
            else {
                node = new_node_binop(ND_ADD, node, parse_mul());
                node->type = type_int();
            }
        }
        else if (consume_token(TK_MINUS)) {
            node = new_node_binop(ND_SUB, node, parse_mul());
            node->type = type_int();
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
            node = new_node_binop(ND_MUL, node, parse_unary());
            node->type = type_int();
        }
        else if (consume_token(TK_DIV)) {
            node = new_node_binop(ND_DIV, node, parse_unary());
            node->type = type_int();
        }
        else {
            return node;
        }
    }
}

static Ast *parse_unary(void)
{
    if (consume_token(TK_MUL)) {
        Ast *node = new_node(ND_DEREF);
        node->unary = parse_unary();
        node->type = type_deref(node->unary->type);
        return node;
    }
    if (consume_token(TK_AND)) {
        Ast *node = new_node(ND_ADDR);
        node->unary = parse_unary();
        node->type = type_ptr(node->unary->type);
        return node;
    }
    if (consume_token(TK_PLUS)) {
        return parse_primary();
    }
    if (consume_token(TK_MINUS)) {
        Ast *node = new_node_binop(ND_SUB, new_node_num(0), parse_primary());
        node->type = type_int();
        return node;
    }
    if (consume_token(TK_SIZEOF)) {
        Ast *arg = parse_unary();
        int val = 0;
        switch (arg->type->type) {
        case T_INT:
            val = 4;
            break;
        case T_PTR:
            val = 8;
            break;
        }
        Ast *node = new_node_num(val);
        node->type = T_INT;
        return node;
    }
    return parse_primary();
}

static Ast *parse_primary(void)
{
    if (consume_token(TK_LPAREN)) {
        Ast *node = parse_expr();
        expect_token(TK_RPAREN);
        return node;
    }

    Token *tok = consume_token(TK_IDENT);
    if (tok) {
        if (consume_token(TK_LPAREN)) {
            Ast *node = new_node(ND_FUNCALL);
            node->func = calloc(tok->len + 1, sizeof(char));
            strncpy(node->func, tok->str, tok->len);
            if (consume_token(TK_RPAREN)) {
                node->args = new_vector();
            }
            else {
                node->args = parse_arglist();
                expect_token(TK_RPAREN);
            }
            // Todo : 関数の戻り値型を node->type にセットする
            return node;
        }

        Ast *node = new_node(ND_LVAR);
        LVar *lvar = find_lvar(tok);
        if (lvar) {
            node->lvar = lvar;
        }
        else {
            error_at(tok->str, "定義されていない変数を使っています");
        }
        node->type = node->lvar->type;
        return node;
    }

    tok = consume_token(TK_NUM);
    if (tok) {
        Ast *node = new_node(ND_NUM);
        node->val = tok->val;
        node->type = type_int();
        return node;
    }

    error("パーズできません");
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

    Type *type = calloc(1, sizeof(Type));
    type->type = T_INT;

    while (consume_token(TK_MUL)) {
        Type *new_type = calloc(1, sizeof(Type));
        new_type->type = T_PTR;
        new_type->ptr_to = type;
        type = new_type;
    }

    return type;
}
