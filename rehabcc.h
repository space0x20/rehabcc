#include <assert.h>
#include <ctype.h>
#include <memory.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// util.c ///////////////////////////////////////

int align(int, int);

// vector.c /////////////////////////////////////

struct vector {
    void **data;
    int size;
    int cap;
};

struct vector *new_vector(void);
void vector_push_back(struct vector *vec, void *data);

// token.c //////////////////////////////////////

enum token_kind {
    TK_PLUS,     // +
    TK_MINUS,    // -
    TK_MUL,      // *
    TK_DIV,      // /
    TK_AND,      // &
    TK_LPAREN,   // (
    TK_RPAREN,   // )
    TK_LBRACE,   // {
    TK_RBRACE,   // }
    TK_LBRACKET, // [
    TK_RBRACKET, // ]
    TK_EQ,       // ==
    TK_NE,       // !=
    TK_LT,       // <
    TK_LE,       // <=
    TK_GT,       // >
    TK_GE,       // >=
    TK_ASSIGN,   // =
    TK_COLON,    // ,
    TK_SCOLON,   // ;
    TK_RETURN,   // return
    TK_IF,       // if
    TK_ELSE,     // else
    TK_WHILE,    // while
    TK_FOR,      // for
    TK_INT,      // int
    TK_SIZEOF,   // sizeof
    TK_NUM,      // 整数
    TK_IDENT,    // 識別子
    TK_EOF,      // 入力終わり
};

struct token {
    enum token_kind kind; // トークン種別
    struct token *next;   // 次のトークン
    char *str;            // トークンの元となる文字列の開始位置
    int len;              // トークンの元となる文字列の長さ
    int val;              // 整数トークンの値
};

struct token *new_token(enum token_kind, struct token *, char *, int);
void set_token(struct token *);
struct token *consume_token(enum token_kind);
struct token *expect_token(enum token_kind);
void error_token(char *, ...);
char *copy_token_str(struct token *);

// type.c ///////////////////////////////////////

enum basic_type {
    T_VOID,
    T_INT,
    T_PTR,
    T_ARRAY,
};

struct type {
    enum basic_type bt;
    struct type *ptr_to;
    int nbyte;
    int array_size;
};

struct type *void_type(void);
struct type *int_type(void);
struct type *ptr_type(struct type *);
struct type *deref_type(struct type *);
struct type *array_type(struct type *, int);

// var.c ////////////////////////////////////////

struct var {
    struct var *next;  // 次のローカル変数またはNULL
    struct type *type; // 変数の型
    char *name;        // ローカル変数名
    int offset;        // RBP からのオフセット
};

void clear_local_vars(void);
struct var *get_local_vars(void);
struct var *add_local_var(struct token *, struct type *);
struct var *find_local_var(struct token *);

// ast.c ////////////////////////////////////////

enum ast_kind {
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
};

struct ast {
    enum ast_kind kind;
    struct type *type;
    struct ast *lhs;
    struct ast *rhs;

    // kind = ND_NUM の場合整数値
    int val;

    // if (cond) { then } else { els }
    // while (cond) { stmt }
    // for (init; cond; update) { stmt }
    struct ast *cond;
    struct ast *then;
    struct ast *els;
    struct ast *stmt;
    struct ast *init;
    struct ast *update;

    // kind = ND_BLOCK
    struct vector *stmts;

    // kind = ND_LVAR の場合に使う
    struct var *var;

    // AST_FUNCTION, AST_FUNCALL
    char *funcname;
    struct var *locals;
    struct vector *params;
};

struct ast *new_ast(enum ast_kind, struct type *);
struct ast *new_ast_unary(enum ast_kind, struct type *, struct ast *);
struct ast *new_ast_binary(enum ast_kind, struct type *, struct ast *, struct ast *);
struct ast *new_ast_num(int val);

// rehabcc.c ////////////////////////////////////

// 入力プログラム
extern char *user_input;

// 構文木列
extern struct ast *code[];

// エラー処理
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// tokenize.c ///////////////////////////////////

void tokenize(void);

// parse.c //////////////////////////////////////

void parse(void);

// generate.c ///////////////////////////////////

void generate(void);
