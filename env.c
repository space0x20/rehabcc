#include "rehabcc.h"

static struct lvar *locals;

void init_lvar(void)
{
    struct lvar *lvar = calloc(1, sizeof(struct lvar));
    lvar->next = NULL;
    lvar->name = NULL;
    lvar->len = 0;
    lvar->offset = 0;
    lvar->type = void_type();
    locals = lvar;
}

struct lvar *get_locals(void)
{
    return locals;
}

struct lvar *add_lvar(struct token *tok, struct type *type)
{
    struct lvar *lvar = calloc(1, sizeof(struct lvar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = align(locals->offset + type->nbyte, 8);
    lvar->type = type;
    locals = lvar;
    return lvar;
}

struct lvar *find_lvar(struct token *tok)
{
    for (struct lvar *lvar = locals; lvar != NULL; lvar = lvar->next) {
        if (lvar->len == tok->len && !memcmp(lvar->name, tok->str, lvar->len)) {
            return lvar;
        }
    }
    return NULL;
}
