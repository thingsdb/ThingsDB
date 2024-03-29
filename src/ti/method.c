/*
 * ti/method.c
 */
#include <tiinc.h>
#include <doc.h>
#include <ti/closure.h>
#include <ti/do.h>
#include <ti/enum.inline.h>
#include <ti/field.h>
#include <ti/method.h>
#include <ti/name.h>
#include <ti/names.h>
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
    ti_name_unsafe_drop(method->name);
    ti_val_unsafe_drop((ti_val_t *) method->closure);
    ti_val_drop((ti_val_t *) method->def);
    ti_val_drop((ti_val_t *) method->doc);
    free(method);
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
        ti_type_t * type_or_enum,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    cleri_node_t * child = nd->children;        /* first in argument list */
    vec_t * args = NULL;
    uint32_t n = method->closure->vars->n;
    ti_ref_t * thing_or_member = (ti_ref_t *) query->rval;
    _Bool lock_was_set = ti_type_ensure_lock(type_or_enum);

    query->rval = NULL;

    if (n)
    {
        args = vec_new(n--);
        if (!args)
        {
            ex_set_mem(e);
            goto fail0;
        }

        VEC_push(args, thing_or_member);
        ti_incref(thing_or_member);

        while (child && n)
        {
            --n;  // outside `while` so we do not go below zero

            if (ti_do_statement(query, child, e))
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
    vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);

fail0:
    ti_type_unlock(type_or_enum, lock_was_set);
    ti_val_unsafe_drop((ti_val_t *) thing_or_member);

    return e->nr;
}

int ti_method_set_name_t(
        ti_method_t * method,
        ti_type_t * type,
        const char * s,
        size_t n,
        ex_t * e)
{
    ti_name_t * name;

    if (!ti_name_is_valid_strn(s, n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "method name must follow the naming rules"DOC_NAMES);
        return e->nr;
    }

    name = ti_names_get(s, n);
    if (!name)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (type->idname == name ||
        ti_field_by_name(type, name) ||
        ti_type_get_method(type, name))
    {
        ex_set(e, EX_VALUE_ERROR,
            "property or method `%s` already exists on type `%s`"DOC_T_TYPED,
            name->str,
            type->name);
        goto fail0;
    }

    ti_name_unsafe_drop(method->name);
    method->name = name;

    return 0;

fail0:
    ti_name_unsafe_drop(name);
    return e->nr;
}

int ti_method_set_name_e(
        ti_method_t * method,
        ti_enum_t * enum_,
        const char * s,
        size_t n,
        ex_t * e)
{
    ti_name_t * name;

    if (!ti_name_is_valid_strn(s, n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "method name must follow the naming rules"DOC_NAMES);
        return e->nr;
    }

    name = ti_names_get(s, n);
    if (!name)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_enum_member_by_strn(enum_, name->str, name->n) ||
        ti_enum_get_method(enum_, name))
    {
        ex_set(e, EX_VALUE_ERROR,
            "member or method `%s` already exists on enum `%s`"DOC_T_TYPED,
            name->str,
            enum_->name);
        goto fail0;
    }

    ti_name_unsafe_drop(method->name);
    method->name = name;

    return 0;

fail0:
    ti_name_unsafe_drop(name);
    return e->nr;
}

void ti_method_set_closure(ti_method_t * method, ti_closure_t * closure)
{
    ti_val_drop((ti_val_t *) method->def);
    ti_val_drop((ti_val_t *) method->doc);
    ti_val_unsafe_drop((ti_val_t *) method->closure);
    method->def = NULL;
    method->doc = NULL;
    method->closure = closure;
    ti_incref(closure);
}
