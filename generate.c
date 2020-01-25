#include "rehabcc.h"

static void emit(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("  ");
    vprintf(fmt, ap);
    printf("\n");
}

void generate(Node *node)
{
    if (node->kind == ND_NUM)
    {
        emit("push %d", node->val);
        return;
    }

    generate(node->lhs);
    generate(node->rhs);

    emit("pop rdi");
    emit("pop rax");

    switch (node->kind)
    {
    case ND_ADD:
        emit("add rax, rdi");
        break;
    case ND_SUB:
        emit("sub rax, rdi");
        break;
    case ND_MUL:
        emit("imul rax, rdi");
        break;
    case ND_DIV:
        emit("cqo");      // 64 bit の rax の値を 128 bit に伸ばして rdx と rax にセットする
        emit("idiv rdi"); // rdx と rax を合わせた 128 bit の数値を rdi で割り算する
        break;
    }

    emit("push rax");
}
