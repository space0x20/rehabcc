#include "rehabcc.h"

static LVar *locals;

void init_lvar(void)
{
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = NULL;
    lvar->name = NULL;
    lvar->len = 0;
    lvar->offset = 0;
    lvar->type = void_type();
    locals = lvar;
}

LVar *get_locals(void)
{
    return locals;
}

LVar *add_lvar(Token *tok, Type *type)
{
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = align(locals->offset + type->nbyte, 8);
    lvar->type = type;
    locals = lvar;
    return lvar;
}

LVar *find_lvar(Token *tok)
{
    for (LVar *lvar = locals; lvar != NULL; lvar = lvar->next) {
        if (lvar->len == tok->len && !memcmp(lvar->name, tok->str, lvar->len)) {
            return lvar;
        }
    }
    return NULL;
}
