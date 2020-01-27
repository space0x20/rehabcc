#include "rehabcc.h"

// 次のトークンが期待している種類のときには、トークンを一つ進めて真を返す。
// そうでない場合は、偽を返す。
static bool consume(TokenKind kind)
{
    if (token->kind != kind)
    {
        return false;
    }
    token = token->next;
    return true;
}

// 着目しているトークンが識別子の場合には、そのトークンを返す。
// さらに着目しているトークンを一つすすめる。
// 着目しているトークンが識別子でない場合は、NULLポインタを返す。
// この場合着目しているトークンは進めない。
static Token *consume_ident(void)
{
    if (token->kind == TK_IDENT)
    {
        Token *tok = token;
        token = token->next;
        return tok;
    }
    return NULL;
}

// 次のトークンが期待している種類のものであれば、トークンを一つ進める。
// それ以外の場合であればエラーを出す。
static void expect(TokenKind kind)
{
    if (token->kind != kind)
    {
        error_at(token->str, "%d ではありません", kind);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを一つ進めて数値を返す。
// それ以外の場合はエラーを出す。
static int expect_number(void)
{
    if (token->kind != TK_NUM)
    {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

// 次のトークンが入力の末尾の場合、真を返す。
static bool at_eof(void)
{
    return token->kind == TK_EOF;
}

// 次のトークンが与えられたトークンかどうかをチェックする
static bool peek(TokenKind kind)
{
    return token->kind == kind;
}

// ローカル変数を追加する
static LVar *add_lvar(Token *tok)
{
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = locals->offset + 8;
    locals = lvar;
    return lvar;
}

// ローカル変数を検索する
// 見つかれば LVar へのポインタ、見つからなければ NULL
static LVar *find_lvar(Token *tok)
{
    for (LVar *lvar = locals; lvar != NULL; lvar = lvar->next)
    {
        if (lvar->len == tok->len && !memcmp(lvar->name, tok->str, lvar->len))
        {
            return lvar;
        }
    }
    return NULL;
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
    return node;
}

// 文法
// program    = stmt*
// stmt       = expr ";"
//            | "{" stmt* "}"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//            | "return" expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = primary ("*" primary | "/" primary)*
// unary      = ("+" | "-")? primary
// primary    = num | ident ("(" ")")? | "(" expr ")"

static void program(void);
static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

void parse(void)
{
    program();
}

static void program(void)
{
    int i = 0;
    while (!at_eof())
    {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

static Node *stmt(void)
{
    if (consume(TK_RETURN))
    {
        Node *node = new_node(ND_RETURN);
        node->ret = expr();
        expect(TK_SCOLON);
        return node;
    }

    if (consume(TK_IF))
    {
        Node *node = new_node(ND_IF);
        expect(TK_LPAREN);
        node->cond = expr();
        expect(TK_RPAREN);
        node->then = stmt();
        node->els = consume(TK_ELSE) ? stmt() : NULL;
        return node;
    }

    if (consume(TK_WHILE))
    {
        Node *node = new_node(ND_WHILE);
        expect(TK_LPAREN);
        node->cond = expr();
        expect(TK_RPAREN);
        node->stmt = stmt();
        return node;
    }

    if (consume(TK_FOR))
    {
        Node *node = new_node(ND_FOR);
        expect(TK_LPAREN);
        if (!peek(TK_SCOLON))
        {
            node->init = expr();
        }
        expect(TK_SCOLON);
        if (!peek(TK_SCOLON))
        {
            node->cond = expr();
        }
        expect(TK_SCOLON);
        if (!peek(TK_RPAREN))
        {
            node->update = expr();
        }
        expect(TK_RPAREN);
        node->stmt = stmt();
        return node;
    }

    if (consume(TK_LBRACE))
    {
        Node *node = new_node(ND_BLOCK);
        node->stmts = new_vector();
        while (!peek(TK_RBRACE))
        {
            push_back(node->stmts, (void *)stmt());
        }
        expect(TK_RBRACE);
        return node;
    }

    Node *node = expr();
    expect(TK_SCOLON);
    return node;
}

static Node *expr(void)
{
    return assign();
}

static Node *assign(void)
{
    Node *node = equality();
    if (consume(TK_ASSIGN))
    {
        node = new_node_binop(ND_ASSIGN, node, assign());
    }
    return node;
}

static Node *equality(void)
{
    Node *node = relational();

    while (1)
    {
        if (consume(TK_EQ))
        {
            node = new_node_binop(ND_EQ, node, relational());
        }
        else if (consume(TK_NE))
        {
            node = new_node_binop(ND_NE, node, relational());
        }
        else
        {
            return node;
        }
    }
}

static Node *relational(void)
{
    Node *node = add();

    while (1)
    {
        if (consume(TK_LT))
        {
            node = new_node_binop(ND_LT, node, add());
        }
        else if (consume(TK_LE))
        {
            node = new_node_binop(ND_LE, node, add());
        }
        else if (consume(TK_GT))
        {
            node = new_node_binop(ND_LT, add(), node);
        }
        else if (consume(TK_GE))
        {
            node = new_node_binop(ND_LE, add(), node);
        }
        else
        {
            return node;
        }
    }
}

static Node *add(void)
{
    Node *node = mul();

    while (1)
    {
        if (consume(TK_PLUS))
        {
            node = new_node_binop(ND_ADD, node, mul());
        }
        else if (consume(TK_MINUS))
        {
            node = new_node_binop(ND_SUB, node, mul());
        }
        else
        {
            return node;
        }
    }
}

static Node *mul(void)
{
    Node *node = unary();

    while (1)
    {
        if (consume(TK_MUL))
        {
            node = new_node_binop(ND_MUL, node, unary());
        }
        else if (consume(TK_DIV))
        {
            node = new_node_binop(ND_DIV, node, unary());
        }
        else
        {
            return node;
        }
    }
}

static Node *unary(void)
{
    if (consume(TK_PLUS))
    {
        return primary();
    }
    if (consume(TK_MINUS))
    {
        return new_node_binop(ND_SUB, new_node_num(0), primary());
    }
    return primary();
}

static Node *primary(void)
{
    if (consume(TK_LPAREN))
    {
        Node *node = expr();
        expect(TK_RPAREN);
        return node;
    }

    Token *tok = consume_ident();
    if (tok)
    {
        if (peek(TK_LPAREN))
        {
            expect(TK_LPAREN);
            Node *node = new_node(ND_FUNCALL);
            node->func = calloc(tok->len + 1, sizeof(char));
            strncpy(node->func, tok->str, tok->len);
            expect(TK_RPAREN);
            return node;
        }

        Node *node = new_node(ND_LVAR);
        LVar *lvar = find_lvar(tok);
        if (lvar)
        {
            node->lvar = lvar;
        }
        else
        {
            lvar = add_lvar(tok);
            node->lvar = lvar;
        }
        return node;
    }

    return new_node_num(expect_number());
}
