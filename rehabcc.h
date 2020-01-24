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

    // トークンの元となる文字列の開始位置と長さ
    char *str;
    int len;

    // TK_NUM の場合の整数値
    int val;
};

// rehabcc.c ////////////////////////////////////

// 現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

// エラー処理
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// tokenize.c ///////////////////////////////////

Token *tokenize(char *p);
