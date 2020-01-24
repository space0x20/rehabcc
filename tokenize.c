#include "rehabcc.h"

typedef struct
{
    TokenKind kind;
    char *str;
} Keyword;

// clang-format off
static Keyword keywords[] = {
    {TK_RESERVED, "+"},
    {TK_RESERVED, "-"},
    {TK_RESERVED, "*"},
    {TK_RESERVED, "/"},
    {TK_RESERVED, "("},
    {TK_RESERVED, ")"},
    {TK_EOF, ""},
};
// clang-format on

// 新しいトークンを作成して cur の次につなげる
Token *
new_token(TokenKind kind, Token *cur, char *str, int len)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列をトークン分割して最初のトークンを返す
Token *tokenize(char *p)
{
    Token head;
    head.next = NULL;
    Token *cur = &head;

loop:
    while (*p)
    {
        // 空白文字をスキップ
        if (isspace(*p))
        {
            p++;
            continue;
        }

        for (int i = 0; keywords[i].kind != TK_EOF; i++)
        {
            int len = strlen(keywords[i].str);
            if (strncmp(p, keywords[i].str, len) == 0)
            {
                cur = new_token(keywords[i].kind, cur, p, len);
                p += len;
                goto loop;
            }
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10); // 10桁まで
            continue;
        }

        error_at(p, "トークン分割できません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
