#include "rehabcc.h"

// 現在着目しているトークン
struct token *token;

// 入力ファイル
char *filename;

// 入力プログラム
char *user_input;

// 構文木列
struct vector *asts;

// 文字列リテラル
struct vector *string_literals;

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
    // loc が含まれている行の開始地点と終了地点
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }
    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    // 見つかった行が全体の何行目なのか
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s:%d: %s\n", filename, line_num, user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

char *read_file(char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        error("cannot open %s: %s", path, strerror(errno));
    }

    if (fseek(fp, 0, SEEK_END) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }

    char *buf = calloc(1, size + 2);
    fread(buf, size, 1, fp);
    if (size == 0 || buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    asts = new_vector();
    string_literals = new_vector();

    // トークン分割
    filename = argv[1];
    user_input = read_file(argv[1]);
    tokenize();
    parse();
    generate();

    return 0;
}
