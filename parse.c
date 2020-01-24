#include "rehabcc.h"

// 次のトークンが期待している種類のときには、トークンを一つ進めて真を返す。
// そうでない場合は、偽を返す。
static bool consume(char op)
{
    if (token->kind != TK_RESERVED || token->str[0] != op)
    {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが期待している種類のものであれば、トークンを一つ進める。
// それ以外の場合であればエラーを出す。
static void expect(char op)
{
    if (token->kind != TK_RESERVED || token->str[0] != op)
    {
        error_at(token->str, "%c ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを一つ進めて数値を返す。
// それ以外の場合はエラーを出す。
static int expect_number()
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
static bool at_eof()
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
// expr    = mul ("+" mul | "-" mul)*
// mul     = primary ("*" primary | "/" primary)*
// unary   = ("+" | "-")? primary
// primary = num | "(" expr ")"
static Node *expr(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

Node *parse()
{
    return expr();
}

static Node *expr()
{
    Node *node = mul();

    while (1)
    {
        if (consume('+'))
        {
            node = new_node(ND_ADD, node, mul());
        }
        else if (consume('-'))
        {
            node = new_node(ND_SUB, node, mul());
        }
        else
        {
            return node;
        }
    }
}

static Node *mul()
{
    Node *node = unary();

    while (1)
    {
        if (consume('*'))
        {
            node = new_node(ND_MUL, node, unary());
        }
        else if (consume('/'))
        {
            node = new_node(ND_DIV, node, unary());
        }
        else
        {
            return node;
        }
    }
}

static Node *unary()
{
    if (consume('+'))
    {
        return primary();
    }
    if (consume('-'))
    {
        return new_node(ND_SUB, new_node_num(0), primary());
    }
    return primary();
}

static Node *primary()
{
    if (consume('('))
    {
        Node *node = expr();
        expect(')');
        return node;
    }

    return new_node_num(expect_number());
}
