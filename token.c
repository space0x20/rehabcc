#include "rehabcc.h"

static Token *token = NULL;

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

void expect_token(TokenKind kind)
{
    if (token->kind != kind) {
        error_at(token->str, "%d ではありません", kind);
    }
    token = token->next;
}

void error_token(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int pos = token->str - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

char *copy_token_str(Token *tok)
{
    char *str = calloc(tok->len + 1, sizeof(char));
    strncpy(str, tok->str, tok->len);
    return str;
}
