/*
 * ti/scope.c
 */
#include <assert.h>
#include <stdlib.h>
#include <langdef/nd.h>
#include <ti/scope.h>
#include <ti/names.h>
#include <ti.h>
#include <util/logger.h>

ti_scope_t * ti_scope_enter(ti_scope_t * scope, ti_thing_t * thing)
{
    ti_scope_t * nscope = malloc(sizeof(ti_scope_t));
    if (!nscope)
        return NULL;

    nscope->prev = scope;
    nscope->thing = ti_grab(thing);
    nscope->name = NULL;
    nscope->val = NULL;
    nscope->iter = NULL;

    return nscope;
}

void ti_scope_leave(ti_scope_t ** scope, ti_scope_t * until)
{
    ti_scope_t * cur = *scope;
    while (cur != until)
    {
        ti_scope_t * prev = cur->prev;

        ti_thing_drop(cur->thing);
        ti_iter_destroy(cur->iter);
        free(cur);

        cur = prev;
    }
    *scope = until;
}

ti_val_t * ti_scope_to_val(ti_scope_t * scope)
{
    if (ti_scope_is_thing(scope))
        return ti_val_create(TI_VAL_THING, scope->thing);
    assert (scope->val);
    return ti_val_dup(scope->val);
}

int ti_scope_push_name(ti_scope_t ** scope, ti_name_t * name, ti_val_t * val)
{
    assert ((*scope)->name == NULL);
    assert ((*scope)->val == NULL);

    ti_scope_t * nscope;

    (*scope)->name = name;
    (*scope)->val = val;
    if (val->tp != TI_VAL_THING)
        return 0;

    nscope = ti_scope_enter(*scope, val->via.thing);
    if (!nscope)
        return -1;

    *scope = nscope;
    return 0;
}

int ti_scope_push_thing(ti_scope_t ** scope, ti_thing_t * thing)
{
    ti_scope_t * nscope = ti_scope_enter(*scope, thing);
    if (!nscope)
        return -1;

    *scope = nscope;
    return 0;
}

_Bool ti_scope_in_use_name(
        ti_scope_t * scope,
        ti_thing_t * thing,
        ti_name_t * name)
{
    while (scope)
    {
        if (scope->thing == thing && scope->name == name)
            return true;
        scope = scope->prev;
    }
    return false;
}

int ti_scope_set_iter_names(ti_scope_t * scope, cleri_node_t * nd, ex_t * e)
{
    assert (langdef_nd_is_function_params(nd));
    assert (scope->iter == NULL);

    cleri_children_t * child;
    int n;
    scope->iter = ti_iter_create();
    if (!scope->iter)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    if (!langdef_nd_is_iterator_param(nd))
    {
        int n = langdef_nd_info_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `filter` expects an `iterator` but %d %s given, "
                "see "TI_DOCS"#iterating",
                n, n == 1 ? "positional argument was" : "were");
        return e->nr;
    }

    child = nd                          /* sequence */
            ->children->node            /* list */
            ->children;                 /* first child */

    for (n = 0;; child = child->next->next, ++n)
    {
        assert (child);
        ti_name_t * name = ti_names_get(child->node->str, child->node->len);
        if (!name)
            goto alloc_err;

        scope->iter[n]->name = name;

        if (!child->next)
            break;
    }

    assert (n);
    assert (n <= 2);

    goto done;

alloc_err:
    ex_set_alloc(e);
done:
    return e->nr;
}

ti_val_t *  ti_scope_iter_val(ti_scope_t * scope, ti_name_t * name)
{
    while (scope)
    {
        ti_val_t * val = ti_iter_get_val(scope->iter, name);
        if (val)
            return val;
        scope = scope->prev;
    }
    return NULL;
}
