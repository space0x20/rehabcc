#include "rehabcc.h"

static struct var *locals;

void clear_local_vars(void)
{
    locals = NULL;
}

struct var *get_local_vars(void)
{
    return locals;
}

struct var *add_local_var(struct token *tok, struct type *type)
{
    struct var *lvar = calloc(1, sizeof(struct var));
    lvar->next = locals;
    lvar->name = copy_token_str(tok);
    lvar->type = type;
    if (locals) {
        lvar->offset = align(locals->offset + type->nbyte, 8);
    }
    else {
        lvar->offset = align(type->nbyte, 8);
    }
    locals = lvar;
    return lvar;
}

struct var *find_local_var(struct token *tok)
{
    for (struct var *lvar = locals; lvar != NULL; lvar = lvar->next) {
        if (!memcmp(lvar->name, tok->str, tok->len)) {
            return lvar;
        }
    }
    return NULL;
}
