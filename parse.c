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

static Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// 文法
// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = primary ("*" primary | "/" primary)*
// unary      = ("+" | "-")? primary
// primary    = num | "(" expr ")"
static Node *expr(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

Node *parse(void)
{
    return expr();
}

static Node *expr(void)
{
    return equality();
}

static Node *equality(void)
{
    Node *node = relational();

    while (1)
    {
        if (consume(TK_EQ))
        {
            node = new_node(ND_EQ, node, relational());
        }
        else if (consume(TK_NE))
        {
            node = new_node(ND_NE, node, relational());
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
            node = new_node(ND_LT, node, add());
        }
        else if (consume(TK_LE))
        {
            node = new_node(ND_LE, node, add());
        }
        else if (consume(TK_GT))
        {
            node = new_node(ND_LT, add(), node);
        }
        else if (consume(TK_GE))
        {
            node = new_node(ND_LE, add(), node);
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
            node = new_node(ND_ADD, node, mul());
        }
        else if (consume(TK_MINUS))
        {
            node = new_node(ND_SUB, node, mul());
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
            node = new_node(ND_MUL, node, unary());
        }
        else if (consume(TK_DIV))
        {
            node = new_node(ND_DIV, node, unary());
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
        return new_node(ND_SUB, new_node_num(0), primary());
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

    return new_node_num(expect_number());
}
