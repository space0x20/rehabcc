#include "rehabcc.h"

static char *regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

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

static void gen(Ast *);

// ノードを左辺値として評価して、スタックにプッシュする
// 左辺値として評価できない場合はエラーとする
static void gen_lval(Ast *node)
{
    switch (node->kind) {
    case AST_LVAR: {
        // 変数には RBP - offset でアクセスできる
        println("  mov rax, rbp");
        println("  sub rax, %d", node->lvar->offset);
        println("  push rax");
        return;
    }
    case AST_DEREF: {
        // *x = 42; を評価する場合、変数 x に格納されている値＝アドレスがほしい。
        // なので、x を右辺値として評価すればいい。
        gen(node->lhs); // スタックトップにほしいアドレスが来る
        return;
    }
    }
    error("左辺値として評価できません");
}

static void gen(Ast *node)
{
    switch (node->kind) {
    case AST_NUM: {
        println("  push %d", node->val);
        return;
    }
    case AST_LVAR: {
        // 変数を右辺値として評価する
        gen_lval(node); // スタックトップに変数のアドレスが来る
        println("  pop rax");
        println("  mov rax, [rax]"); // rax に変数の値を読み出す
        println("  push rax");
        return;
    }
    case AST_ASSIGN: {
        gen_lval(node->lhs);
        gen(node->rhs);
        println("  pop rdi"); // 右辺値
        println("  pop rax"); // 左辺アドレス
        println("  mov [rax], rdi");
        println("  push rdi"); // 代入式の評価値は右辺値
        return;
    }
    case AST_RETURN: {
        gen(node->lhs); // return 式の値を評価、スタックトップに式の値が残る
        println("  pop rax");
        // 呼び出し元の関数フレームに戻る
        println("  mov rsp, rbp");
        println("  pop rbp");
        println("  ret");
        return;
    }
    case AST_IF: {
        int label = get_label();
        gen(node->cond);
        println("  pop rax");
        println("  cmp rax, 0");
        if (node->els == NULL) {
            println("  je .Lend%d", label);
            gen(node->then);
            println(".Lend%d:", label);
        }
        else {
            println("  je .Lelse%d", label);
            gen(node->then);
            println("  jmp .Lend%d", label);
            println(".Lelse%d:", label);
            gen(node->els);
            println(".Lend%d:", label);
        }
        return;
    }
    case AST_WHILE: {
        int label = get_label();
        println(".Lbegin%d:", label);
        gen(node->cond);
        println("  pop rax");
        println("  cmp rax, 0");
        println("  je .Lend%d", label);
        gen(node->stmt);
        println("  jmp .Lbegin%d", label);
        println(".Lend%d:", label);
        return;
    }
    case AST_FOR: {
        int label = get_label();
        if (node->init) {
            gen(node->init);
        }
        println(".Lbegin%d:", label);
        if (node->cond) {
            gen(node->cond);
            println("  pop rax");
            println("  cmp rax, 0");
            println("  je .Lend%d", label);
        }
        gen(node->stmt);
        if (node->update) {
            gen(node->update);
        }
        println("  jmp .Lbegin%d", label);
        println(".Lend%d:", label);
        return;
    }
    case AST_BLOCK: {
        for (int i = 0; i < node->stmts->size; i++) {
            gen(node->stmts->data[i]);
        }
        return;
    }
    case AST_FUNCALL: {
        int label = get_label();
        // 引数をレジスタに載せる
        for (int i = 0; i < node->args->size; i++) {
            if (i == 6) {
                // まだ6個までしか渡せない
                break;
            }
            gen(node->args->data[i]); // スタックトップに引数を評価した値が来る
            println("  pop rax");
            println("  mov %s, rax", regs[i]);
        }
        // call 命令の時点で rsp を 16 バイト境界に揃える
        println("  mov rax, rsp");
        println("  and rax, 15");
        println("  je .Lcall%d", label);
        println("  sub rsp, 8");
        println("  call %s", node->func);
        println("  add rsp, 8");
        println("  jmp .Lend%d", label);
        println(".Lcall%d:", label);
        println("  call %s", node->func);
        println(".Lend%d:", label);
        println("  push rax"); // 関数の戻り値をスタックトップに載せる
        return;
    }
    case AST_FUNCTION: {
        println("%s:", node->funcname);
        println("  push rbp");
        println("  mov rbp, rsp");
        println("  sub rsp, %d", node->locals->offset);

        // 引数をスタックにコピーする
        for (int i = 0; i < node->params->size; i++) {
            println("  mov [rbp - %d], %s", 8 * (i + 1), regs[i]);
        }

        // 本体のコード生成
        for (int i = 0; i < node->stmts->size; i++) {
            gen(node->stmts->data[i]);
        }

        // 関数のエピローグ
        println("  mov rsp, rbp");
        println("  pop rbp");
        println("  ret");
        return;
    }
    case AST_ADDR: {
        gen_lval(node->lhs); // lhs を左辺値として評価すればアドレスが手に入る
        return;
    }
    case AST_DEREF: {
        gen(node->lhs); // スタックトップにアドレスが入る
        println("  pop rax");
        println("  mov rax, [rax]");
        println("  push rax");
        return;
    }
    case AST_VARDECL: {
        return;
    }
    }

    gen(node->lhs);
    gen(node->rhs);

    println("  pop rdi");
    println("  pop rax");

    switch (node->kind) {
    case AST_ADD:
        println("  add rax, rdi");
        break;
    case AST_SUB:
        println("  sub rax, rdi");
        break;
    case AST_MUL:
        println("  imul rax, rdi");
        break;
    case AST_DIV:
        println("  cqo");      // 64 bit の rax の値を 128 bit に伸ばして rdx と rax にセットする
        println("  idiv rdi"); // rdx と rax を合わせた 128 bit の数値を rdi で割り算する
        break;
    case AST_EQ:
        println("  cmp rax, rdi");  // rax と rdi の比較
        println("  sete al");       // cmp の結果を al レジスタ (rax の下位 8 bit) に設定する
        println("  movzb rax, al"); // rax の上位 56 bit をゼロで埋める
        break;
    case AST_NE:
        println("  cmp rax, rdi");
        println("  setne al");
        println("  movzb rax, al");
        break;
    case AST_LT:
        println("  cmp rax, rdi");
        println("  setl al");
        println("  movzb rax, al");
        break;
    case AST_LE:
        println("  cmp rax, rdi");
        println("  setle al");
        println("  movzb rax, al");
        break;
    case AST_ADD_PTR: {
        println("  imul rdi, %d", node->lhs->lvar->type->ptr_to->nbyte);
        println("  add rax, rdi");
        break;
    }
    }

    println("  push rax");
}

void generate(void)
{
    // アセンブリの前半部分
    println(".intel_syntax noprefix");
    println(".global main");
    for (int i = 0; code[i] != NULL; i++) {
        gen(code[i]);
    }
}
