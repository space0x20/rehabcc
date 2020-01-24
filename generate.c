#include "rehabcc.h"

void generate(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("  push %d\n", node->val);
        return;
    }

    generate(node->lhs);
    generate(node->rhs);

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
