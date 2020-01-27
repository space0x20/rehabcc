#include "rehabcc.h"

typedef struct
{
    TokenKind kind;
    char *str;
} TokenMap;

// clang-format off
// トークンとして認識する記号たち
// 列挙順にマッチングを行うため、たとえば <= を < よりも先に並べる必要がある
static TokenMap symbols[] = {
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

static TokenMap keywords[] = {
    {TK_RETURN, "return"},
    {TK_IF, "if"},
    {TK_ELSE, "else"},
    {TK_WHILE, "while"},
    {TK_FOR, "for"},
    {TK_EOF, ""}, // 最後においておくこと
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

// 識別子として受け入れる文字かどうかをチェックする
static bool isident(char c)
{
    return isalnum(c) || c == '_';
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
        for (int i = 0; symbols[i].kind != TK_EOF; i++)
        {
            int len = strlen(symbols[i].str);
            if (strncmp(p, symbols[i].str, len) == 0)
            {
                cur = new_token(symbols[i].kind, cur, p, len);
                p += len;
                goto loop;
            }
        }

        // キーワードのトークン化
        for (int i = 0; keywords[i].kind != TK_EOF; i++)
        {
            int len = strlen(keywords[i].str);
            if (strncmp(p, keywords[i].str, len) == 0 && !isident(p[len]))
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
        if (isident(*p))
        {
            char *q = p;
            while (isident(*q))
            {
                q++;
            }
            int len = q - p;
            cur = new_token(TK_IDENT, cur, p, len);
            p = q;
            continue;
        }

        error_at(p, "トークン分割できません");
    }

    new_token(TK_EOF, cur, p, 0);
    token = head.next;
}
