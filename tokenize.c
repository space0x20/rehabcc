#include "rehabcc.h"

struct token_kind_str {
    enum token_kind kind;
    char *str;
};

// clang-format off
// トークンとして認識する記号たち
// 列挙順にマッチングを行うため、たとえば <= を < よりも先に並べる必要がある
static struct token_kind_str symbols[] = {
    {TK_PLUS, "+"},
    {TK_MINUS, "-"},
    {TK_MUL, "*"},
    {TK_DIV, "/"},
    {TK_AND, "&"},
    {TK_LPAREN, "("},
    {TK_RPAREN, ")"},
    {TK_LBRACE, "{"},
    {TK_RBRACE, "}"},
    {TK_LBRACKET, "["},
    {TK_RBRACKET, "]"},
    {TK_EQ, "=="},
    {TK_NE, "!="},
    {TK_LE, "<="},
    {TK_LT, "<"},
    {TK_GE, ">="},
    {TK_GT, ">"},
    {TK_ASSIGN, "="},
    {TK_COLON, ","},
    {TK_SCOLON, ";"},
    {TK_EOF, ""},  // 最後においておくこと
};

static struct token_kind_str keywords[] = {
    {TK_RETURN, "return"},
    {TK_SIZEOF, "sizeof"},
    {TK_IF, "if"},
    {TK_ELSE, "else"},
    {TK_WHILE, "while"},
    {TK_FOR, "for"},
    {TK_INT, "int"},
    {TK_EOF, ""}, // 最後においておくこと
};
// clang-format on

// 識別子として受け入れる文字かどうかをチェックする
static bool isident(char c)
{
    return isalnum(c) || c == '_';
}

// 入力文字列をトークン分割して最初のトークンを返す
void tokenize(void)
{
    struct token head;
    head.next = NULL;
    struct token *cur = &head;
    char *p = user_input;

loop:
    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 記号のトークン化
        for (int i = 0; symbols[i].kind != TK_EOF; i++) {
            int len = strlen(symbols[i].str);
            if (strncmp(p, symbols[i].str, len) == 0) {
                cur = new_token(symbols[i].kind, cur, p, len);
                p += len;
                goto loop;
            }
        }

        // キーワードのトークン化
        for (int i = 0; keywords[i].kind != TK_EOF; i++) {
            int len = strlen(keywords[i].str);
            if (strncmp(p, keywords[i].str, len) == 0 && !isident(p[len])) {
                cur = new_token(keywords[i].kind, cur, p, len);
                p += len;
                goto loop;
            }
        }

        // 整数トークン
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10); // 10桁まで
            continue;
        }

        // 識別子トークン
        if (isident(*p)) {
            char *q = p;
            while (isident(*q)) {
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
    set_token(head.next);
}
