#include "rehabcc.h"

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

void gen(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind)
    {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");      // 64 bit の rax の値を 128 bit に伸ばして rdx と rax にセットする
        printf("  idiv rdi\n"); // rdx と rax を合わせた 128 bit の数値を rdi で割り算する
        break;
    }

    printf("  push rax\n");
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
    Node *node = parse();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // アセンブリコード生成
    gen(node);

    printf("  pop rax\n"); // スタックトップに式全体の値が残っているはず
    printf("  ret\n");
    return 0;
}
