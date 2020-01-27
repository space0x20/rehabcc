#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Token ////////////////////////////////////////

// トークンの種類
typedef enum
{
    TK_PLUS,   // +
    TK_MINUS,  // -
    TK_MUL,    // *
    TK_DIV,    // /
    TK_LPAREN, // (
    TK_RPAREN, // )
    TK_EQ,     // ==
    TK_NE,     // !=
    TK_LT,     // <
    TK_LE,     // <=
    TK_GT,     // >
    TK_GE,     // >=
    TK_ASSIGN, // =
    TK_SCOLON, // ;
    TK_RETURN, // return
    TK_IF,     // if
    TK_ELSE,   // else
    TK_NUM,    // 整数
    TK_IDENT,  // 識別子
    TK_EOF,    // 入力終わり
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

// LVar /////////////////////////////////////////

typedef struct LVar LVar;

struct LVar
{
    LVar *next; // 次のローカル変数またはNULL
    char *name; // ローカル変数名
    int len;    // 変数名の長さ
    int offset; // RBP からのオフセット
};

// Node /////////////////////////////////////////

// 抽象構文木のノードの種類
typedef enum
{
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_ASSIGN,
    ND_RETURN,
    ND_IF,
    ND_LVAR, // ローカル変数
} NodeKind;

typedef struct Node Node;

struct Node
{
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;

    // kind = ND_RETURN
    Node *ret;

    // kind = ND_IF
    Node *cond;
    Node *then;
    Node *els;

    // kind = ND_LVAR の場合に使う
    LVar *lvar;
};

// rehabcc.c ////////////////////////////////////

// 現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

// 構文木列
extern Node *code[];

// ローカル変数の連結リスト
extern LVar *locals;

// エラー処理
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// tokenize.c ///////////////////////////////////

void tokenize(void);

// parse.c //////////////////////////////////////

void parse(void);

// generate.c ///////////////////////////////////

void generate(void);
