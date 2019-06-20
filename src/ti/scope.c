/*
 * ti/scope.c
 */
#include <assert.h>
#include <stdlib.h>
#include <langdef/nd.h>
#include <ti/scope.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/vint.h>
#include <ti.h>
#include <util/logger.h>

ti_scope_t * ti_scope_enter(ti_scope_t * scope, ti_thing_t * thing)
{
    ti_scope_t * nscope = malloc(sizeof(ti_scope_t));
    if (!nscope)
        return NULL;

    nscope->prev = scope;
    nscope->thing = ti_grab(thing);
    nscope->name = nscope->val = NULL;
    nscope->local = NULL;

    return nscope;
}

void ti_scope_leave(ti_scope_t ** scope, ti_scope_t * until)
{
    ti_scope_t * cur = *scope;
    while (cur != until)
    {
        ti_scope_t * prev = cur->prev;

        ti_val_drop((ti_val_t *) cur->thing);
        vec_destroy(cur->local, (vec_destroy_cb) ti_prop_destroy);
        free(cur);

        cur = prev;
    }
    *scope = until;
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
    {
        if (scope->thing == thing && scope->name == name)
            return true;
    }
    return false;
}

_Bool ti_scope_in_use_val(
        ti_scope_t * scope,
        ti_thing_t * thing,
        ti_val_t * val)
{
    for (; scope; scope = scope->prev)
    {
        if (scope->thing == thing && scope->val == val)
            return true;
    }
    return false;
}

int ti_scope_local_from_closure(
        ti_scope_t * scope,
        ti_closure_t * closure,
        ex_t * e)
{
    size_t n;
    cleri_children_t * child, * first;

    if (scope->local)
    {
        /* cleanup existing local */
        vec_destroy(scope->local, (vec_destroy_cb) ti_prop_destroy);
        scope->local = NULL;
    }

    first = closure->node               /* sequence */
            ->children->next->node      /* list */
            ->children;                 /* first child */

    for (n = 0, child = first; child && ++n; child = child->next->next)
        if (!child->next)
            break;

    scope->local = vec_new(n);
    if (!scope->local)
        goto alloc_err;

    for (child = first; child; child = child->next->next)
    {
        ti_val_t * val = (ti_val_t *) ti_nil_get();
        ti_name_t * name = ti_names_get(child->node->str, child->node->len);
        ti_prop_t * prop;
        if (!name)
            goto alloc_err;

        prop = ti_prop_create(name, val);
        if (!prop)
        {
            ti_name_drop(name);
            ti_val_drop(val);
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
                    return prop->val;
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
            return prop->val;
    return NULL;
}

int ti_scope_polute_prop(ti_scope_t * scope, ti_prop_t * prop)
{
    size_t n = 0;
    for (vec_each(scope->local, ti_prop_t, p), ++n)
    {
        ti_val_drop(p->val);
        switch (n)
        {
        case 0:
            p->val = (ti_val_t *) prop->name;
            ti_incref(p->val);
            break;
        case 1:
            p->val = prop->val;
            ti_incref(p->val);
            break;
        default:
            p->val = (ti_val_t *) ti_nil_get();
        }
    }
    return 0;
}

int ti_scope_polute_val(ti_scope_t * scope, ti_val_t * val, int64_t idx)
{
    size_t n = 0;
    for (vec_each(scope->local, ti_prop_t, p), ++n)
    {
       ti_val_drop(p->val);
       switch (n)
       {
       case 0:
           p->val = val;
           ti_incref(p->val);
           break;
       case 1:
           p->val = (ti_val_t *) ti_vint_create(idx);
           if (!p->val)
               return -1;
           break;
       default:
           p->val = (ti_val_t *) ti_nil_get();
       }
    }
    return 0;
}

