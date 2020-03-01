#include "rehabcc.h"

static struct var *locals = NULL;
static struct var *globals = NULL;

static struct var *add_var(struct var *head, struct token *tok, struct type *type)
{
    struct var *var = calloc(1, sizeof(struct var));
    var->next = head;
    var->name = copy_token_str(tok);
    var->type = type;
    return var;
}

static struct var *find_var(struct var *head, struct token *tok)
{
    for (struct var *var = head; var != NULL; var = var->next) {
        if (!memcmp(var->name, tok->str, tok->len)) {
            return var;
        }
    }
    return NULL;
}

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
    struct var *var = add_var(locals, tok, type);
    if (locals) {
        var->offset = align(locals->offset + type->nbyte, 8);
    }
    else {
        var->offset = align(type->nbyte, 8);
    }
    locals = var;
    return var;
}

struct var *find_local_var(struct token *tok)
{
    return find_var(locals, tok);
}

struct var *get_global_vars(void)
{
    return globals;
}

struct var *add_global_var(struct token *tok, struct type *type)
{
    globals = add_var(globals, tok, type);
    return globals;
}

struct var *find_global_var(struct token *tok)
{
    return find_var(globals, tok);
}
