#include "rehabcc.h"

// 着目しているトークンが型の場合には、その型を返す
// そうでない場合には NULL ポインタを返す
static Type *consume_type(void)
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

static Node *new_node(NodeKind kind)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_node_binop(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val)
{
    Node *node = new_node(ND_NUM);
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

static void program(void);
static Node *funcdef(void);
static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);
static Vector *arglist(void);
static Type *typedecl(void);

void parse(void)
{
    program();
}

static void program(void)
{
    int i = 0;
    while (!consume_token(TK_EOF)) {
        code[i++] = funcdef();
    }
    code[i] = NULL;
}

static Node *funcdef(void)
{
    Node *node = new_node(ND_FUNCDEF);

    node->type = consume_type();
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
        Type *type = consume_type();
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
        push_back(node->stmts, stmt());
    }

    node->locals = locals;
    return node;
}

static Node *stmt(void)
{
    if (consume_token(TK_RETURN)) {
        Node *node = new_node(ND_RETURN);
        node->ret = expr();
        expect_token(TK_SCOLON);
        return node;
    }

    if (consume_token(TK_IF)) {
        Node *node = new_node(ND_IF);
        expect_token(TK_LPAREN);
        node->cond = expr();
        expect_token(TK_RPAREN);
        node->then = stmt();
        node->els = consume_token(TK_ELSE) ? stmt() : NULL;
        return node;
    }

    if (consume_token(TK_WHILE)) {
        Node *node = new_node(ND_WHILE);
        expect_token(TK_LPAREN);
        node->cond = expr();
        expect_token(TK_RPAREN);
        node->stmt = stmt();
        return node;
    }

    if (consume_token(TK_FOR)) {
        Node *node = new_node(ND_FOR);
        expect_token(TK_LPAREN);
        if (!consume_token(TK_SCOLON)) {
            node->init = expr();
            expect_token(TK_SCOLON);
        }
        if (!consume_token(TK_SCOLON)) {
            node->cond = expr();
            expect_token(TK_SCOLON);
        }
        if (!consume_token(TK_RPAREN)) {
            node->update = expr();
            expect_token(TK_RPAREN);
        }
        node->stmt = stmt();
        return node;
    }

    if (consume_token(TK_LBRACE)) {
        Node *node = new_node(ND_BLOCK);
        node->stmts = new_vector();
        while (!consume_token(TK_RBRACE)) {
            push_back(node->stmts, (void *)stmt());
        }
        return node;
    }

    Type *type = consume_type();
    if (type) {
        Node *node = new_node(ND_VARDECL);
        Token *tok = consume_token(TK_IDENT);
        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            add_lvar(tok, type);
        }
        expect_token(TK_SCOLON);
        return node;
    }

    Node *node = expr();
    expect_token(TK_SCOLON);
    return node;
}

static Node *expr(void)
{
    return assign();
}

static Node *assign(void)
{
    Node *node = equality();
    if (consume_token(TK_ASSIGN)) {
        node = new_node_binop(ND_ASSIGN, node, assign());
        node->type = node->lhs->type;
    }
    return node;
}

static Node *equality(void)
{
    Node *node = relational();

    while (1) {
        if (consume_token(TK_EQ)) {
            node = new_node_binop(ND_EQ, node, relational());
            node->type = type_int();
        }
        else if (consume_token(TK_NE)) {
            node = new_node_binop(ND_NE, node, relational());
            node->type = type_int();
        }
        else {
            return node;
        }
    }
}

static Node *relational(void)
{
    Node *node = add();

    while (1) {
        if (consume_token(TK_LT)) {
            node = new_node_binop(ND_LT, node, add());
            node->type = type_int();
        }
        else if (consume_token(TK_LE)) {
            node = new_node_binop(ND_LE, node, add());
            node->type = type_int();
        }
        else if (consume_token(TK_GT)) {
            node = new_node_binop(ND_LT, add(), node);
            node->type = type_int();
        }
        else if (consume_token(TK_GE)) {
            node = new_node_binop(ND_LE, add(), node);
            node->type = type_int();
        }
        else {
            return node;
        }
    }
}

static Node *add(void)
{
    Node *node = mul();

    while (1) {
        if (consume_token(TK_PLUS)) {
            if (node->kind == ND_LVAR && node->lvar->type->type == T_PTR) {
                node = new_node_binop(ND_ADD_PTR, node, mul());
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
                node = new_node_binop(ND_ADD, node, mul());
                node->type = type_int();
            }
        }
        else if (consume_token(TK_MINUS)) {
            node = new_node_binop(ND_SUB, node, mul());
            node->type = type_int();
        }
        else {
            return node;
        }
    }
}

static Node *mul(void)
{
    Node *node = unary();

    while (1) {
        if (consume_token(TK_MUL)) {
            node = new_node_binop(ND_MUL, node, unary());
            node->type = type_int();
        }
        else if (consume_token(TK_DIV)) {
            node = new_node_binop(ND_DIV, node, unary());
            node->type = type_int();
        }
        else {
            return node;
        }
    }
}

static Node *unary(void)
{
    if (consume_token(TK_MUL)) {
        Node *node = new_node(ND_DEREF);
        node->unary = unary();
        node->type = type_deref(node->unary->type);
        return node;
    }
    if (consume_token(TK_AND)) {
        Node *node = new_node(ND_ADDR);
        node->unary = unary();
        node->type = type_ptr(node->unary->type);
        return node;
    }
    if (consume_token(TK_PLUS)) {
        return primary();
    }
    if (consume_token(TK_MINUS)) {
        Node *node = new_node_binop(ND_SUB, new_node_num(0), primary());
        node->type = type_int();
        return node;
    }
    if (consume_token(TK_SIZEOF)) {
        Node *arg = unary();
        int val = 0;
        switch (arg->type->type) {
        case T_INT:
            val = 4;
            break;
        case T_PTR:
            val = 8;
            break;
        }
        Node *node = new_node_num(val);
        node->type = T_INT;
        return node;
    }
    return primary();
}

static Node *primary(void)
{
    if (consume_token(TK_LPAREN)) {
        Node *node = expr();
        expect_token(TK_RPAREN);
        return node;
    }

    Token *tok = consume_token(TK_IDENT);
    if (tok) {
        if (consume_token(TK_LPAREN)) {
            Node *node = new_node(ND_FUNCALL);
            node->func = calloc(tok->len + 1, sizeof(char));
            strncpy(node->func, tok->str, tok->len);
            if (consume_token(TK_RPAREN)) {
                node->args = new_vector();
            }
            else {
                node->args = arglist();
                expect_token(TK_RPAREN);
            }
            // Todo : 関数の戻り値型を node->type にセットする
            return node;
        }

        Node *node = new_node(ND_LVAR);
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
        Node *node = new_node(ND_NUM);
        node->val = tok->val;
        node->type = type_int();
        return node;
    }

    error("パーズできません");
}

static Vector *arglist(void)
{
    Vector *args = new_vector();
    push_back(args, expr());
    while (consume_token(TK_COLON)) {
        push_back(args, expr());
    }
    return args;
}
