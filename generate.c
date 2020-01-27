#include "rehabcc.h"

static void emit(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("  ");
    vprintf(fmt, ap);
    printf("\n");
}

static void println(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\n");
}

static int get_label()
{
    static int label = 0;
    label++;
    return label;
}

// ノードを左辺値として評価して、スタックにプッシュする
// 左辺値として評価できない場合はエラーとする
static void gen_lval(Node *node)
{
    if (node->kind != ND_LVAR)
    {
        error("代入の左辺値が変数ではありません");
    }
    // 変数には RBP - offset でアクセスできる
    emit("mov rax, rbp");
    emit("sub rax, %d", node->lvar->offset);
    emit("push rax");
}

static void gen(Node *node)
{
    switch (node->kind)
    {
    case ND_NUM:
        emit("push %d", node->val);
        return;
    case ND_LVAR:
        // 変数を右辺値として評価する
        gen_lval(node); // スタックトップに変数のアドレスが来る
        emit("pop rax");
        emit("mov rax, [rax]"); // rax に変数の値を読み出す
        emit("push rax");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);
        emit("pop rdi"); // 右辺値
        emit("pop rax"); // 左辺アドレス
        emit("mov [rax], rdi");
        emit("push rdi"); // 代入式の評価値は右辺値
        return;
    case ND_RETURN:
        gen(node->ret); // return 式の値を評価、スタックトップに式の値が残る
        emit("pop rax");
        // 呼び出し元の関数フレームに戻る
        emit("mov rsp, rbp");
        emit("pop rbp");
        emit("ret");
        return;
    }

    if (node->kind == ND_IF)
    {
        int label = get_label();
        gen(node->cond);
        emit("pop rax");
        emit("cmp rax, 0");
        if (node->els == NULL)
        {
            emit("je .Lend%d", label);
            gen(node->then);
            println(".Lend%d:", label);
        }
        else
        {
            emit("je .Lelse%d", label);
            gen(node->then);
            emit("jmp .Lend%d", label);
            println(".Lelse%d:", label);
            gen(node->els);
            println(".Lend%d:", label);
        }
        return;
    }

    if (node->kind == ND_WHILE)
    {
        int label = get_label();
        println(".Lbegin%d:", label);
        gen(node->cond);
        emit("pop rax");
        emit("cmp rax, 0");
        emit("je .Lend%d", label);
        gen(node->stmt);
        emit("jmp .Lbegin%d", label);
        println(".Lend%d:", label);
        return;
    }

    if (node->kind == ND_FOR)
    {
        int label = get_label();
        if (node->init)
        {
            gen(node->init);
        }
        println(".Lbegin%d:", label);
        if (node->cond)
        {
            gen(node->cond);
            emit("pop rax");
            emit("cmp rax, 0");
            emit("je .Lend%d", label);
        }
        gen(node->stmt);
        if (node->update)
        {
            gen(node->update);
        }
        emit("jmp .Lbegin%d", label);
        println(".Lend%d:", label);
        return;
    }

    if (node->kind == ND_BLOCK)
    {
        for (int i = 0; i < node->stmts->size; i++)
        {
            gen((Node *)(node->stmts->data[i]));
        }
        return;
    }

    if (node->kind == ND_FUNCALL)
    {
        int label = get_label();
        // call 命令の時点で rsp を 16 バイト境界に揃える
        emit("mov rax, rsp");
        emit("and rax, 15");
        emit("je .Lcall%d", label);
        emit("sub rsp, 8");
        println(".Lcall%d:", label);
        emit("call %s", node->func);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

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
    case ND_EQ:
        emit("cmp rax, rdi");  // rax と rdi の比較
        emit("sete al");       // cmp の結果を al レジスタ (rax の下位 8 bit) に設定する
        emit("movzb rax, al"); // rax の上位 56 bit をゼロで埋める
        break;
    case ND_NE:
        emit("cmp rax, rdi");
        emit("setne al");
        emit("movzb rax, al");
        break;
    case ND_LT:
        emit("cmp rax, rdi");
        emit("setl al");
        emit("movzb rax, al");
        break;
    case ND_LE:
        emit("cmp rax, rdi");
        emit("setle al");
        emit("movzb rax, al");
        break;
    }

    emit("push rax");
}

void generate(void)
{
    // アセンブリの前半部分
    println(".intel_syntax noprefix");
    println(".global main");
    println("main:");

    // プロローグ
    // 変数26個分をスタックに確保
    emit("push rbp");
    emit("mov rbp, rsp");
    emit("sub rsp, 208");

    // 最初の式から順にコード生成
    for (int i = 0; code[i] != NULL; i++)
    {
        gen(code[i]);
        emit("pop rax"); // 途中式の評価結果は逐次捨てる
    }

    // エピローグ
    emit("mov rsp, rbp");
    emit("pop rbp");
    emit("ret");
}
