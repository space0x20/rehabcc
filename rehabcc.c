#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum
{
    TK_RESERVED, // 記号
    TK_NUM,      // 整数
    TK_EOF,      // 入力終わり
} TokenKind;

// トークン型
typedef struct Token Token;
struct Token
{
    TokenKind kind;
    Token *next;
    int val;
    char *str;
};

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している種類のときには、トークンを一つ進めて真を返す。
// そうでない場合は、偽を返す。
bool consume(char op)
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
void expect(char op)
{
    if (token->kind != TK_RESERVED || token->str[0] != op)
    {
        error_at(token->str, "%c ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを一つ進めて数値を返す。
// それ以外の場合はエラーを出す。
int expect_number()
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
bool at_eof()
{
    return token->kind == TK_EOF;
}

// 新しいトークンを作成して cur の次につなげる
Token *new_token(TokenKind kind, Token *cur, char *str)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

// 入力文字列をトークン分割して最初のトークンを返す
Token *tokenize(char *p)
{
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p)
    {
        // 空白文字をスキップ
        if (isspace(*p))
        {
            p++;
            continue;
        }

        if (strchr("+-", *p))
        {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10); // 10桁まで
            continue;
        }

        error_at(p, "トークン分割できません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // トークン分割
    user_input = argv[1];
    token = tokenize(argv[1]);

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    printf("  mov rax, %d\n", expect_number());
    while (!at_eof())
    {
        if (consume('+'))
        {
            printf("  add rax, %d\n", expect_number());
            continue;
        }
        expect('-');
        printf("  sub rax, %d\n", expect_number());
    }

    printf("  ret\n");
    return 0;
}
