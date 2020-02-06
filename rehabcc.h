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

// token.c //////////////////////////////////////

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

Token *new_token(TokenKind, Token *, char *, int);
void set_token(Token *);
Token *consume_token(TokenKind);
void expect_token(TokenKind);

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
    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV,
    AST_NUM,
    AST_EQ,
    AST_NE,
    AST_LT,
    AST_LE,
    AST_ASSIGN,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_BLOCK,
    AST_FUNCALL,  // 関数呼び出し
    AST_FUNCTION, // 関数定義
    AST_ADDR,     // リファレンス &
    AST_DEREF,    // デリファレンス *
    AST_VARDECL,  // 変数宣言
    AST_LVAR,     // ローカル変数
    AST_ADD_PTR,  // ポインタの足し算
} AstKind;

typedef struct Ast Ast;

struct Ast {
    AstKind kind;
    Ast *lhs;
    Ast *rhs;

    // kind = ND_NUM の場合整数値
    int val;

    // kind = ND_RETURN
    Ast *ret;

    // if (cond) { then } else { els }
    // while (cond) { stmt }
    // for (init; cond; update) { stmt }
    Ast *cond;
    Ast *then;
    Ast *els;
    Ast *stmt;
    Ast *init;
    Ast *update;

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
    Ast *unary;

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
extern Ast *code[];

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
