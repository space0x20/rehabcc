#include "rehabcc.h"

// 現在着目しているトークン
struct token *token;

// 入力プログラム
char *user_input;

// 構文木列
struct ast *code[100];

// ローカル変数
struct lvar *locals;

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

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    locals = calloc(1, sizeof(struct lvar));
    locals->next = NULL;
    locals->name = "";
    locals->len = 0;
    locals->offset = 0;

    // トークン分割
    user_input = argv[1];
    tokenize();
    parse();
    generate();

    return 0;
}
