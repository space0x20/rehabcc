#include "rehabcc.h"

Token *token = NULL;

Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

void set_token(Token *t)
{
    token = t;
}

Token *consume_token(TokenKind kind)
{
    if (token->kind == kind) {
        Token *t = token;
        token = token->next;
        return t;
    }
    return NULL;
}
