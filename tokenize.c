#include "rehabcc.h"

typedef struct
{
    TokenKind kind;
    char *str;
} Keyword;

// clang-format off
// トークンとして認識する記号たち
// 列挙順にマッチングを行うため、たとえば <= を < よりも先に並べる必要がある
static Keyword keywords[] = {
    {TK_PLUS, "+"},
    {TK_MINUS, "-"},
    {TK_MUL, "*"},
    {TK_DIV, "/"},
    {TK_LPAREN, "("},
    {TK_RPAREN, ")"},
    {TK_EQ, "=="},
    {TK_NE, "!="},
    {TK_LE, "<="},
    {TK_LT, "<"},
    {TK_GE, ">="},
    {TK_GT, ">"},
    {TK_ASSIGN, "="},
    {TK_SCOLON, ";"},
    {TK_EOF, ""},  // 最後においておくこと
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
void tokenize(void)
{
    Token head;
    head.next = NULL;
    Token *cur = &head;
    char *p = user_input;

loop:
    while (*p)
    {
        // 空白文字をスキップ
        if (isspace(*p))
        {
            p++;
            continue;
        }

        // 記号のトークン化
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

        // 整数トークン
        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10); // 10桁まで
            continue;
        }

        // 識別子トークン
        if (islower(*p))
        {
            cur = new_token(TK_IDENT, cur, p, 1);
            p += 1;
            continue;
        }

        error_at(p, "トークン分割できません");
    }

    new_token(TK_EOF, cur, p, 0);
    token = head.next;
}
