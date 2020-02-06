#include <ctype.h>
#include <memory.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// util.c ///////////////////////////////////////

typedef struct Vector Vector;

struct Vector {
    void **data;
    int size;
    int cap;
};

Vector *new_vector(void);
void push_back(Vector *vec, void *data);

// token.h //////////////////////////////////////

typedef enum {
    TK_PLUS,   // +
    TK_MINUS,  // -
    TK_MUL,    // *
    TK_DIV,    // /
    TK_AND,    // &
    TK_LPAREN, // (
    TK_RPAREN, // )
    TK_LBRACE, // {
    TK_RBRACE, // }
    TK_EQ,     // ==
    TK_NE,     // !=
    TK_LT,     // <
    TK_LE,     // <=
    TK_GT,     // >
    TK_GE,     // >=
    TK_ASSIGN, // =
    TK_COLON,  // ,
    TK_SCOLON, // ;
    TK_RETURN, // return
    TK_IF,     // if
    TK_ELSE,   // else
    TK_WHILE,  // while
    TK_FOR,    // for
    TK_INT,    // int
    TK_SIZEOF, // sizeof
    TK_NUM,    // 整数
    TK_IDENT,  // 識別子
    TK_EOF,    // 入力終わり
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind; // トークン種別
    Token *next;    // 次のトークン
    char *str;      // トークンの元となる文字列の開始位置
    int len;        // トークンの元となる文字列の長さ
    int val;        // 整数トークンの値
};

extern Token *token;

Token *new_token(TokenKind, Token *, char *, int);
void set_token(Token *);

// Type /////////////////////////////////////////

typedef enum {
    T_INT,
    T_PTR,
} BasicType;

typedef struct Type Type;

struct Type {
    BasicType type;
    Type *ptr_to;
};

// LVar /////////////////////////////////////////

typedef struct LVar LVar;

struct LVar {
    LVar *next; // 次のローカル変数またはNULL
    char *name; // ローカル変数名
    int len;    // 変数名の長さ
    int offset; // RBP からのオフセット
    Type *type; // 変数の型
};

// Node /////////////////////////////////////////

// 抽象構文木のノードの種類
typedef enum {
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
    ND_WHILE,
    ND_FOR,
    ND_BLOCK,
    ND_FUNCALL,
    ND_FUNCDEF,
    ND_ADDR,    // リファレンス &
    ND_DEREF,   // デリファレンス *
    ND_VARDECL, // 変数宣言
    ND_LVAR,    // ローカル変数
    ND_ADD_PTR, // ポインタの足し算
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;

    // kind = ND_NUM の場合整数値
    int val;

    // kind = ND_RETURN
    Node *ret;

    // if (cond) { then } else { els }
    // while (cond) { stmt }
    // for (init; cond; update) { stmt }
    Node *cond;
    Node *then;
    Node *els;
    Node *stmt;
    Node *init;
    Node *update;

    // kind = ND_BLOCK
    Vector *stmts;

    // kind = ND_LVAR の場合に使う
    LVar *lvar;

    // kind = ND_FUNC
    char *funcname;
    Vector *params; // 変数名のベクトル (型情報は今はない)
    LVar *locals;

    // kind = ND_FUNCALL
    char *func;
    Vector *args; // vector of Node (expr)

    // kind = ND_DEREF
    Node *unary;

    // kind = ND_FUNCDECL
    // kind = ND_LVAR
    Type *type;

    // kind = ND_ADD_PTR
    // ポインタと整数の足し算の場合、データ型のサイズ
    int type_size;
};

// rehabcc.c ////////////////////////////////////

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
char *copy_str(Token *tok);

// parse.c //////////////////////////////////////

void parse(void);

// generate.c ///////////////////////////////////

void generate(void);
