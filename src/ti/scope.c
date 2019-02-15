/*
 * ti/scope.c
 */
#include <assert.h>
#include <stdlib.h>
#include <langdef/nd.h>
#include <ti/scope.h>
#include <ti/prop.h>
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
    nscope->local = NULL;
    return nscope;
}

void ti_scope_leave(ti_scope_t ** scope, ti_scope_t * until)
{
    ti_scope_t * cur = *scope;
    while (cur != until)
    {
        ti_scope_t * prev = cur->prev;

        ti_thing_drop(cur->thing);
        vec_destroy(cur->local, (vec_destroy_cb) ti_prop_weak_destroy);
        free(cur);

        cur = prev;
    }
    *scope = until;
}

/*
 * Returns a new referenced value.
 * In case of an error the return value is NULL.
 */
ti_val_t * ti_scope_global_to_val(ti_scope_t * scope)
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

_Bool ti_scope_in_use_thing(ti_scope_t * scope, ti_thing_t * thing)
{
    for (; scope; scope = scope->prev)
        if (scope->thing == thing)
            return true;
    return false;
}

_Bool ti_scope_in_use_name(
        ti_scope_t * scope,
        ti_thing_t * thing,
        ti_name_t * name)
{
    for (; scope; scope = scope->prev)
        if (scope->thing == thing && scope->name == name)
            return true;
    return false;
}

int ti_scope_local_from_arrow(ti_scope_t * scope, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARROW);
    size_t n;
    cleri_children_t * child, * first;

    if (scope->local)
    {
        /* cleanup existing local */
        vec_destroy(scope->local, (vec_destroy_cb) ti_prop_weak_destroy);
        scope->local = NULL;
    }

    first = nd                          /* sequence */
            ->children->node            /* list */
            ->children;                 /* first child */

    for (n = 0, child = first; child && ++n; child = child->next->next)
        if (!child->next)
            break;

    scope->local = vec_new(n);
    if (!scope->local)
        goto alloc_err;

    for (child = first; child; child = child->next->next)
    {
        ti_prop_t * prop;
        ti_name_t * name = ti_names_get(child->node->str, child->node->len);
        if (!name)
            goto alloc_err;

        prop = ti_prop_create(name, TI_VAL_NIL, NULL);
        if (!prop)
        {
            ti_name_drop(name);
            goto alloc_err;
        }
        VEC_push(scope->local, prop);
        if (!child->next)
            break;
    }

    goto done;

alloc_err:
    ex_set_alloc(e);

done:
    return e->nr;
}

/* find local value in the total scope */
ti_val_t *  ti_scope_find_local_val(ti_scope_t * scope, ti_name_t * name)
{
    while (scope)
    {
        if (scope->local)
            for (vec_each(scope->local, ti_prop_t, prop))
                if (prop->name == name)
                    return &prop->val;
        scope = scope->prev;
    }
    return NULL;
}

/* local value in the current scope */
ti_val_t * ti_scope_local_val(ti_scope_t * scope, ti_name_t * name)
{
    /* we have to look one level up since there the local scope is polluted */
    if (!scope->prev || !scope->prev->local)
        return NULL;
    for (vec_each(scope->prev->local, ti_prop_t, prop))
        if (prop->name == name)
            return &prop->val;
    return NULL;
}
