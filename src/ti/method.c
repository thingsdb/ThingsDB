/*
 * ti/method.c
 */
#include <tiinc.h>
#include <ti/closure.h>
#include <ti/do.h>
#include <ti/method.h>
#include <ti/name.h>
#include <ti/nil.h>
#include <ti/type.h>
#include <ti/val.h>
#include <ti/val.inline.h>

ti_method_t * ti_method_create(ti_name_t * name, ti_closure_t * closure)
{
    ti_method_t * method = malloc(sizeof(ti_method_t));
    if (!method)
        return NULL;
    method->name = name;
    method->closure = closure;
    method->def = NULL;
    method->doc = NULL;

    ti_incref(name);
    ti_incref(closure);

    return method;
}

void ti_method_destroy(ti_method_t * method)
{
    ti_name_drop(method->name);
    ti_val_drop((ti_val_t *) method->closure);
    ti_val_drop((ti_val_t *) method->def);
    ti_val_drop((ti_val_t *) method->doc);
    free(method);
}

ti_method_t * ti_method_by_name(ti_type_t * type, ti_name_t * name)
{
    for (vec_each(type->methods, ti_method_t, method))
        if (method->name == name)
            return method;
    return NULL;
}

/* may return an empty string but never NULL */
ti_raw_t * ti_method_doc(ti_method_t * method)
{
    if (!method->doc)
        method->doc = ti_closure_doc(method->closure);
    return method->doc;
}

/* may return an empty string but never NULL */
ti_raw_t * ti_method_def(ti_method_t * method)
{
    if (!method->def)
        /* calculate only once */
        method->def = ti_closure_def(method->closure);
    return method->def;
}

int ti_method_call(
        ti_method_t * method,
        ti_type_t * type,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    cleri_children_t * child = nd->children;    /* first in argument list */
    vec_t * args = NULL;
    uint32_t n = method->closure->vars->n;
    ti_thing_t * thing = (ti_thing_t *) query->rval;
    _Bool lock_was_set = ti_type_ensure_lock(type);

    query->rval = NULL;

    if (n)
    {
        args = vec_new(n--);
        if (!args)
        {
            ex_set_mem(e);
            goto fail0;
        }

        VEC_push(args, thing);
        ti_incref(thing);

        while (child && n)
        {
            --n;  // outside `while` so we do not go below zero

            if (ti_do_statement(query, child->node, e) ||
                ti_val_make_variable(&query->rval, e))
                goto fail1;

            VEC_push(args, query->rval);
            query->rval = NULL;

            if (!child->next)
                break;

            child = child->next->next;
        }

        while (n--)
            VEC_push(args, ti_nil_get());
    }

    (void) ti_closure_call(method->closure, query, args, e);

fail1:
    vec_destroy(args, (vec_destroy_cb) ti_val_drop);

fail0:
    ti_type_unlock(type, lock_was_set);
    ti_val_drop((ti_val_t *) thing);

    return e->nr;
}
