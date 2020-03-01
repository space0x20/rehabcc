#include "rehabcc.h"

static struct token *token = NULL;

struct token *new_token(enum token_kind kind, struct token *cur, char *str, int len)
{
    struct token *tok = calloc(1, sizeof(struct token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

struct token *get_token(void)
{
    return token;
}

void set_token(struct token *t)
{
    token = t;
}

struct token *consume_token(enum token_kind kind)
{
    if (token->kind == kind) {
        struct token *t = token;
        token = token->next;
        return t;
    }
    return NULL;
}

struct token *expect_token(enum token_kind kind)
{
    if (token->kind == kind) {
        struct token *t = token;
        token = token->next;
        return t;
    }
    else {
        error_at(token->str, "%d ではありません", kind);
    }
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

char *copy_token_str(struct token *tok)
{
    char *str = calloc(tok->len + 1, sizeof(char));
    strncpy(str, tok->str, tok->len);
    return str;
}

void debug_print_token(void)
{
    char *str = copy_token_str(token);
    fprintf(stderr, "token = %d\n", token->kind);
    fprintf(stderr, "str = %s\n", str);
}
