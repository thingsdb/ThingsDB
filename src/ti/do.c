/*
 * ti/do.c
 */
#include <cleri/cleri.h>
#include <doc.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <ti/auth.h>
#include <ti/do.h>
#include <ti/enums.inline.h>
#include <ti/fn/fn.h>
#include <ti/fn/fncall.h>
#include <ti/index.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr/oprinc.h>
#include <ti/regex.h>
#include <ti/task.h>
#include <ti/template.h>
#include <ti/thing.inline.h>
#include <ti/member.inline.h>
#include <ti/vint.h>
#include <ti/preopr.h>
#include <util/strx.h>


static inline int do__no_collection_scope(ti_query_t * query)
{
    return ~query->qbind.flags & TI_QBIND_FLAG_COLLECTION;
}

static int do__array(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARRAY);
    assert (query->rval == NULL);

    ti_varr_t * varr;
    uintptr_t sz = (uintptr_t) nd->data;
    cleri_children_t * child = nd          /* sequence */
            ->children->next->node         /* list */
            ->children;

    varr = ti_varr_create(sz);
    if (!varr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (; child; child = child->next->next)
    {
        if (ti_do_statement(query, child->node, e) ||
            ti_varr_append(varr, (void **) &query->rval, e))
            goto failed;

        query->rval = NULL;

        if (!child->next)
            break;
    }

    query->rval = (ti_val_t *) varr;
    return 0;

failed:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static inline ti_name_t * do__cache_name(ti_query_t * query, cleri_node_t * nd)
{
    assert (nd->data == NULL);
    ti_name_t * name = nd->data = ti_names_weak_get(nd->str, nd->len);
    if (name)
    {
        ti_incref(name);
        VEC_push(query->val_cache, name);
    }
    return name;
}

static inline int do__o_get_wprop(
        ti_wprop_t * wprop,
        ti_query_t * query,
        ti_thing_t * thing,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_prop_t * prop = nd->data
            ? ti_thing_o_prop_weak_get(thing, nd->data)
            : (do__cache_name(query, nd)
                ? ti_thing_o_prop_weak_get(thing, nd->data)
                : NULL);

    if (prop)
    {
        wprop->name = prop->name;
        wprop->val = &prop->val;
        return 0;
    }

    wprop->name = NULL;
    wprop->val = NULL;

    ex_set(e, EX_LOOKUP_ERROR,
            "thing "TI_THING_ID" has no property `%.*s`",
            thing->id, (int) nd->len, nd->str);
    return e->nr;
}

static inline int do__t_get_wprop(
        ti_wprop_t * wprop,
        ti_query_t * query,
        ti_thing_t * thing,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_field_t * field;
    ti_type_t * type = ti_thing_type(thing);
    ti_name_t * name = nd->data ? nd->data : do__cache_name(query, nd);

    if (name && (field = ti_field_by_name(type, name)))
    {
        wprop->name = field->name;
        wprop->val = (ti_val_t **) vec_get_addr(thing->items, field->idx);
        return 0;
    }

    wprop->name = NULL;
    wprop->val = NULL;

    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no property `%.*s`",
            type->name, (int) nd->len, nd->str);
    return e->nr;
}

static inline int do__get_wprop(
        ti_wprop_t * wprop,
        ti_query_t * query,
        ti_thing_t * thing,
        cleri_node_t * nd,
        ex_t * e)
{
    return ti_thing_is_object(thing)
            ? do__o_get_wprop(wprop, query, thing, nd, e)
            : do__t_get_wprop(wprop, query, thing, nd, e);
}

static inline int do__o_upd_prop(
        ti_wprop_t * wprop,
        ti_query_t * query,
        ti_thing_t * thing,
        cleri_node_t * name_nd,
        cleri_node_t * tokens_nd,
        ex_t * e)
{
    return (
        do__o_get_wprop(wprop, query, thing, name_nd, e) ||
        ti_opr_a_to_b(*wprop->val, tokens_nd, &query->rval, e)
    ) ? e->nr : 0;
}

static inline int do__t_upd_prop(
        ti_wprop_t * wprop,
        ti_query_t * query,
        ti_thing_t * thing,
        cleri_node_t * name_nd,
        cleri_node_t * tokens_nd,
        ex_t * e)
{
    ti_field_t * field;
    ti_type_t * type = ti_thing_type(thing);
    ti_name_t * name = name_nd->data ? name_nd->data : do__cache_name(query, name_nd);

    if (name && (field = ti_field_by_name(type, name)))
    {
        wprop->name = field->name;
        wprop->val = (ti_val_t **) vec_get_addr(thing->items, field->idx);

        return (
            ti_opr_a_to_b(*wprop->val, tokens_nd, &query->rval, e) ||
            ti_field_make_assignable(field, &query->rval, thing, e)
        ) ? e->nr : 0;
    }

    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no property `%.*s`",
            type->name, (int) name_nd->len, name_nd->str);
    return e->nr;
}

static inline int do__upd_prop(
        ti_wprop_t * wprop,
        ti_query_t * query,
        ti_thing_t * thing,
        cleri_node_t * name_nd,
        cleri_node_t * tokens_nd,
        ex_t * e)
{
    if (ti_thing_is_object(thing)
            ? do__o_upd_prop(wprop, query, thing, name_nd, tokens_nd, e)
            : do__t_upd_prop(wprop, query, thing, name_nd, tokens_nd, e))
        return e->nr;

    ti_val_drop(*wprop->val);
    *wprop->val = query->rval;
    ti_incref(query->rval);

    return 0;
}

static int do__name_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_NAME_OPT_MORE);
    assert (query->rval);

    ti_thing_t * thing;
    ti_task_t * task;
    ti_wprop_t wprop;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_children_t * assign_seq = nd                  /* sequence */
            ->children->next->node->children;           /* first child */
    cleri_node_t * tokens_nd = assign_seq->node;        /* tokens */

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR, "cannot assign properties to `%s` type",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, assign_seq->next->node, e))
        goto done;

    if (tokens_nd->len == 2
            ? do__upd_prop(&wprop, query, thing, name_nd, tokens_nd, e)
            : ti_thing_set_val_from_valid_strn(
                    &wprop,
                    thing,
                    name_nd->str,
                    name_nd->len,
                    &query->rval,
                    e))
        goto done;

    if (thing->id)
    {
        assert (query->collection);  /* only in a collection scope */
        task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto done;

        if (ti_task_add_set(task, wprop.name, *wprop.val))
            ex_set_mem(e);
    }

done:
    ti_val_unlock((ti_val_t *) thing, true /* lock_was_set */);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static inline ti_prop_t * do__get_var(ti_query_t * query, cleri_node_t * nd)
{
    return nd->data
            ? ti_query_var_get(query, nd->data)
            : (do__cache_name(query, nd)
                ? ti_query_var_get(query, nd->data)
                : NULL);
}

static inline ti_prop_t * do__get_var_e(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_prop_t * prop = do__get_var(query, nd);
    if (!prop)
        ex_set(e, EX_LOOKUP_ERROR,
                "variable `%.*s` is undefined",
                (int) nd->len, nd->str);
    return prop;
}

static int do__function_call(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_node_t * fname = nd       /* sequence */
            ->children->node;       /* name node */

    cleri_node_t * args = nd        /* sequence */
        ->children->next->node      /* function sequence */
        ->children->next->node;     /* arguments */

    if (query->rval)
    {
        if (ti_val_is_thing(query->rval))
        {
            ti_wprop_t wprop;
            ti_thing_t * thing = (ti_thing_t *) query->rval;

            if (do__get_wprop(&wprop, query, thing, fname, e))
                return e->nr;

            if (!ti_val_is_closure(*wprop.val))
            {
                ex_set(e, EX_TYPE_ERROR,
                    "property `%.*s` is of type `%s` and is not callable",
                    fname->len,
                    fname->str,
                    ti_val_str(*wprop.val));
                return e->nr;
            }

            query->rval = *wprop.val;
            ti_incref(query->rval);
            ti_val_drop((ti_val_t *) thing);

            return fn_call(query, args, e);
        }

        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `%.*s`",
                ti_val_str(query->rval),
                fname->len,
                fname->str);
    }
    else
    {
        ti_prop_t * prop = do__get_var(query, fname);
        if (prop)
        {
            if (!ti_val_is_closure(prop->val))
            {
                ex_set(e, EX_TYPE_ERROR,
                    "variable `%.*s` is of type `%s` and is not callable",
                    fname->len,
                    fname->str,
                    ti_val_str(prop->val));
                return e->nr;
            }

            query->rval = prop->val;
            ti_incref(query->rval);

            return fn_call(query, args, e);
        }

        ex_set(e, EX_LOOKUP_ERROR,
                "function `%.*s` is undefined",
                fname->len,
                fname->str);
    }
    return e->nr;
}

static inline int do__function(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->children->next->node->cl_obj->gid == CLERI_GID_FUNCTION);

    return nd->data
            ? ((fn_cb) nd->data)(
                    query,
                    nd                              /* sequence */
                        ->children->next->node      /* function sequence */
                        ->children->next->node,     /* arguments */
                    e)
            : do__function_call(query, nd, e);
}

static int do__block(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_BLOCK);

    uint32_t prev_block_stack = query->block_stack;
    cleri_children_t * child= nd->children->next->next
            ->node->children;  /* first child, not empty */

    query->block_stack = query->vars->n;
    do
    {
        if (ti_do_statement(query, child->node, e))
            return e->nr;

        if (!child->next || !(child = child->next->next))
            break;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }
    while (1);

    while (query->vars->n > query->block_stack)
        ti_query_var_drop_gc(VEC_pop(query->vars), query);

    query->block_stack = prev_block_stack;
    return e->nr;
}

static inline int do__index(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_children_t * child = nd->children;

    for (child = nd->children; child; child = child->next)
        if (ti_index(query, child->node, e))
            return e->nr;
    return 0;
}

static int do__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);
    assert (query->rval);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */
    cleri_node_t * node = child->node;      /* function, assignment, name */
    cleri_node_t * index_node = child->next->node;

    child = child->next->next;          /* set to chain child (or NULL) */

    if (!node->children->next)
    {
        ti_wprop_t wprop;
        ti_thing_t * thing;

        if (!ti_val_is_thing(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR, "type `%s` has no properties",
                    ti_val_str(query->rval));
            return e->nr;
        }

        thing = (ti_thing_t *) query->rval;

        if (do__get_wprop(&wprop, query, thing, node->children->node, e))
            return e->nr;

        query->rval = *wprop.val;
        ti_incref(query->rval);
        ti_val_drop((ti_val_t *) thing);
    }
    else switch (node->children->next->node->cl_obj->gid)
    {
    case CLERI_GID_ASSIGN:
        /* nothing is possible after assign since it ends with a scope */
        return do__name_assign(query, node, e);
    case CLERI_GID_FUNCTION:
        if (do__function(query, node, e))
            return e->nr;
    } /* no other case statements are possible */

    if (do__index(query, index_node, e) == 0 && child)
        (void) do__chain(query, child->node, e);

    return e->nr;
}

int ti_do_operations(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert( nd->cl_obj->gid == CLERI_GID_OPERATIONS );
    assert (query->rval == NULL);

    switch (nd->children->next->node->cl_obj->gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_AND:
    case CLERI_GID_OPR3_BITWISE_XOR:
    case CLERI_GID_OPR4_BITWISE_OR:
    case CLERI_GID_OPR5_COMPARE:
    {
        ti_val_t * a;

        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        a = query->rval;
        query->rval = NULL;

        if (ti_do_statement(query, nd->children->next->next->node, e) == 0)
            (void) ti_opr_a_to_b(a, nd->children->next->node, &query->rval, e);

        ti_val_drop(a);
        return e->nr;
    }
    case CLERI_GID_OPR6_CMP_AND:
        if (    ti_do_statement(query, nd->children->node, e) ||
                !ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return ti_do_statement(query, nd->children->next->next->node, e);
    case CLERI_GID_OPR7_CMP_OR:
        if (    ti_do_statement(query, nd->children->node, e) ||
                ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return ti_do_statement(query, nd->children->next->next->node, e);
    case CLERI_GID_OPR8_TERNARY:
        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        nd = ti_val_as_bool(query->rval)
                ? nd->children->next->node->children->next->node
                : nd->children->next->next->node;

        ti_val_drop(query->rval);
        query->rval = NULL;

        return ti_do_statement(query, nd, e);
    }

    assert (0);
    return e->nr;
}

static int do__thing_by_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (!nd->data)
    {
        int64_t thing_id = strtoll(nd->str + 1, NULL, 10);
        ti_thing_t * thing = ti_query_thing_from_id(query, thing_id, e);
        if (!thing)
            return e->nr;

        nd->data = thing;
        VEC_push(query->val_cache, thing);
    }
    else if (ti_val_is_int((ti_val_t *) nd->data))
    {
        /*
         * Unbound closures do not cache `#` syntax. if we really wanted this,
         * then it could be achieved by assigning the closure instead of `int`
         * to this node cache. But the hard part is then the garbage collection
         * because things keep attached to the stored closure but this is not
         * detected by the current garbage collector.
         */
        ti_vint_t * thing_id = nd->data;
        query->rval = (ti_val_t *) ti_query_thing_from_id(
                query,
                thing_id->int_,
                e);
        return e->nr;
    }

    query->rval = nd->data;
    ti_incref(query->rval);

    return e->nr;
}

static int do__read_closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (!nd->data)
    {
        nd->data = (ti_val_t *) ti_closure_from_node(
                nd,
                (query->qbind.flags & TI_QBIND_FLAG_THINGSDB)
                    ? TI_VFLAG_CLOSURE_BTSCOPE
                    : TI_VFLAG_CLOSURE_BCSCOPE);
        if (!nd->data)
        {
            ex_set_mem(e);
            return e->nr;
        }
        assert (vec_space(query->val_cache));
        VEC_push(query->val_cache, nd->data);
    }
    query->rval = nd->data;
    ti_incref(query->rval);

    return e->nr;
}

static int do__fixed_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_VAR);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    int i
        = langdef_nd_match_str(nd, "READ")
        ? TI_AUTH_READ
        : langdef_nd_match_str(nd, "MODIFY")
        ? TI_AUTH_MODIFY
        : langdef_nd_match_str(nd, "WATCH")
        ? TI_AUTH_WATCH
        : langdef_nd_match_str(nd, "RUN")
        ? TI_AUTH_RUN
        : langdef_nd_match_str(nd, "GRANT")
        ? TI_AUTH_GRANT
        : langdef_nd_match_str(nd, "FULL")
        ? TI_AUTH_MASK_FULL
        : langdef_nd_match_str(nd, "DEBUG")
        ? LOGGER_DEBUG
        : langdef_nd_match_str(nd, "INFO")
        ? LOGGER_INFO
        : langdef_nd_match_str(nd, "WARNING")
        ? LOGGER_WARNING
        : langdef_nd_match_str(nd, "ERROR")
        ? LOGGER_ERROR
        : langdef_nd_match_str(nd, "CRITICAL")
        ? LOGGER_CRITICAL
        : -1;

    if (i < 0)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "variable `%.*s` is undefined",
                (int) nd->len, nd->str);
    }
    else
    {
        query->rval = (ti_val_t *) ti_vint_create(i);
        if (!query->rval)
            ex_set_mem(e);
    }

    return e->nr;
}

static int do__thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence('{', List(Sequence(name, ':', scope)), '}')
     */
    assert (e->nr == 0);

    ti_thing_t * thing;
    cleri_children_t * child;
    uintptr_t sz = (uintptr_t) nd->data;

    thing = ti_thing_o_create(0, sz, query->collection);
    if (!thing)
        goto failed;

    child = nd                                  /* sequence */
            ->children->next->node              /* list */
            ->children;                         /* list items */

    for (; child; child = child->next->next)
    {
        cleri_node_t * name_nd;
        cleri_node_t * scope;
        ti_name_t * name;

        name_nd = child->node                       /* sequence */
                ->children->node;                   /* name */

        scope = child->node                         /* sequence */
                ->children->next->next->node;       /* scope */

        name = ti_names_get(name_nd->str, name_nd->len);
        if (    !name||
                ti_do_statement(query, scope, e) ||
                ti_val_make_assignable(&query->rval, thing, name, e) ||
                !ti_thing_o_prop_set(thing, name, query->rval))
        {
            ti_name_drop(name);
            goto failed;
        }

        query->rval = NULL;

        if (!child->next)
            break;
    }

    query->rval = (ti_val_t *) thing;
    return 0;

failed:
    if (!e->nr)
        ex_set_mem(e);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__instance(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence(Name, Sequence('{', List(Sequence(name, ':', scope)), '}')
     */
    assert (e->nr == 0);

    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    ti_thing_t * thing;
    cleri_children_t * child;
    size_t n;
    ti_type_t * type;
    int lock_was_set;

    if (!query->collection)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "no types exists in the `%s` scope; "
                "you might want to query a `@collection` scope?",
                ti_query_scope_name(query));
        return e->nr;
    }

    type = ti_types_by_strn(
            query->collection->types,
            name_nd->str,
            name_nd->len);
    if (!type)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%.*s` is undefined",
                name_nd->len,
                name_nd->str);
        return e->nr;
    }

    thing = ti_thing_t_create(0, type, query->collection);
    if (!thing)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (n = type->fields->n; n--;)
        VEC_push(thing->items, NULL);

    lock_was_set = ti_type_ensure_lock(type);

    child = nd                                  /* sequence (var_opt_more) */
            ->children->next->node              /* sequence (instance) */
            ->children->next->node              /* list */
            ->children;                         /* list items */

    for (n = 0; child; child = child->next ? child->next->next: NULL)
    {
        cleri_node_t * name_nd;
        cleri_node_t * scope;
        ti_name_t * name;
        ti_field_t * field;
        ti_val_t * val;

        name_nd = child->node                       /* sequence */
                ->children->node;                   /* name */

        scope = child->node                         /* sequence */
                ->children->next->next->node;       /* scope */

        name = ti_names_weak_get(name_nd->str, name_nd->len);
        if (!name)
            continue;

        field = ti_field_by_name(type, name);
        if (!field)
            continue;

        if (    ti_do_statement(query, scope, e) ||
                ti_field_make_assignable(field, &query->rval, thing, e))
            goto fail;

        val = vec_get(thing->items, field->idx);
        if (val)
            ti_val_drop(val);
        else
            ++n;
        vec_set(thing->items, query->rval, field->idx);

        query->rval = NULL;
    }

    if (n < type->fields->n)
    {
        /* fill missing fields */
        for (vec_each(type->fields, ti_field_t, field))
        {
            if (!vec_get(thing->items, field->idx))
            {
                ti_val_t * val = ti_field_dval(field);
                if (!val)
                {
                    ex_set_mem(e);
                    goto fail;
                }

                ti_val_attach(val, thing, field->name);
                vec_set(thing->items, val, field->idx);
            }
        }
    }

    query->rval = (ti_val_t *) thing;
    goto done;

fail:
    ti_val_drop((ti_val_t *) thing);

done:
    ti_type_unlock(type, lock_was_set);
    return e->nr;
}

static int do__enum_get(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_enum_t * enum_;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * enum_nd = nd                 /* sequence */
            ->children->next->node;             /* enum node */

    nd = enum_nd->children->next->node;         /* name or closure */

    enum_ = ti_enums_by_strn(
            query->collection->enums,
            name_nd->str,
            name_nd->len);

    if (!enum_)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%.*s` is undefined",
                name_nd->len,
                name_nd->str);
        return e->nr;
    }

    switch(nd->cl_obj->gid)
    {
    case CLERI_GID_NAME:
        enum_nd->data = (ti_val_t *) ti_enum_member_by_strn(
                enum_,
                nd->str,
                nd->len);
        if (enum_nd->data)
        {
            /*
             * In an unbound closure we have space reserved to store the enum
             * member in the closure cache. This means that the member will be
             * in use from this point on, until the closure is removed.
             */
            vec_t * vec = nd->data ? nd->data : query->val_cache;
            query->rval = enum_nd->data;
            VEC_push(vec, query->rval);
            query->rval->ref += 2;
        }
        else
            ex_set(e, EX_LOOKUP_ERROR, "enum `%s` has no member `%.*s`",
                    enum_->name,
                    nd->len,
                    nd->str);
        break;
    case CLERI_GID_T_CLOSURE:
    {
        ti_closure_t * closure;
        vec_t * args = NULL;
        ti_raw_t * rname;

        if (do__read_closure(query, nd, e))
            return e->nr;

        closure = (ti_closure_t *) query->rval;

        if (closure->vars->n)
        {
            uint32_t n = closure->vars->n;
            args = vec_new(n);
            if (!args)
            {
                ex_set_mem(e);
                return e->nr;
            }

            while (n--)
                VEC_push(args, ti_nil_get());
        }

        query->rval = NULL;
        (void) ti_closure_call(closure, query, args, e);

        /* cleanup closure and arguments */
        ti_val_drop((ti_val_t *) closure);
        vec_destroy(args, (vec_destroy_cb) ti_val_drop);

        if (e->nr)
            return e->nr;

        if (!ti_val_is_str(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "enumerator lookup is expecting type `"TI_VAL_STR_S"` "
                    "but got type `%s` instead"DOC_T_ENUM,
                    ti_val_str(query->rval));
            return e->nr;
        }

        rname = (ti_raw_t *) query->rval;
        query->rval = (ti_val_t *) ti_enum_member_by_raw(enum_, rname);

        if (query->rval)
            ti_incref(query->rval);
        else
            ex_set(e, EX_LOOKUP_ERROR, "enum `%s` has no member `%.*s`",
                    enum_->name,
                    (int) rname->n,
                    (const char *) rname->data);

        ti_val_drop((ti_val_t *) rname);
        break;
    }
    }

    return e->nr;
}

static inline void do__clear_enum_cache(cleri_node_t * enum_nd)
{
    ti_member_t * member = enum_nd->data;
    vec_t * vec = enum_nd->children->next->node->data;
    uint32_t idx = 0;

    for (vec_each(vec, void, data), ++idx)
        if (data == member)
            break;

    assert (idx < vec->n);
    vec_swap_remove(vec, idx);
    ti_member_drop(member);
    enum_nd->data = NULL;
}

static inline int do__enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_node_t * enum_nd = nd                 /* sequence */
            ->children->next->node;             /* enum node */

    if (enum_nd->data)
    {
        ti_member_t * member = enum_nd->data;
        cleri_node_t * name_nd = enum_nd->children->next->node;

        /* enum_ is set to NULL when the enum is removed;
         * the name must be checked to support a rename of the member name;
         */
        if (member->enum_ &&
            member->name->n == name_nd->len &&
            !memcmp(member->name->str, name_nd->str, name_nd->len))
        {
            ti_incref(member);
            query->rval = (ti_val_t *) member;
            return 0;
        }
        do__clear_enum_cache(enum_nd);
    }
    return do__enum_get(query, nd, e);
}

/* changes scope->name and/or scope->thing */
static inline int do__var(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_VAR);
    assert (query->rval == NULL);

    ti_prop_t * prop = do__get_var_e(query, nd, e);

    if (!prop)
    {
        if (do__no_collection_scope(query))
        {
            e->nr = 0;
            return do__fixed_name(query, nd, e);
        }
        return e->nr;
    }

    query->rval = prop->val;
    ti_incref(query->rval);

    return e->nr;
}

static inline ti_prop_t * do__prop_scope(ti_query_t * query, ti_name_t * name)
{
    register uint32_t n = query->vars->n;
    register uint32_t end = query->block_stack;

    while (n-- > end)
    {
        ti_prop_t * prop = vec_get(query->vars, n);
        if (prop->name == name)
            return prop;
    }
    return NULL;
}

static inline ti_name_t * do__ensure_name_cache(
        ti_query_t * query,
        cleri_node_t * nd)
{
    assert (nd->data == NULL);
    ti_name_t * name = nd->data = ti_names_get(nd->str, nd->len);
    if (name)
        VEC_push(query->val_cache, name);
    return name;
}

static int do__var_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_VAR_OPT_MORE);
    assert (query->rval == NULL);

    ti_name_t * name = NULL;
    ti_prop_t * prop = NULL;     /* assign to prevent warning */
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_children_t * assign_seq = nd                  /* sequence */
            ->children->next->node->children;           /* first child */
    cleri_node_t * tokens_nd = assign_seq->node;        /* sequence */

    if (ti_do_statement(query, assign_seq->next->node, e))
        return e->nr;

    if (tokens_nd->len == 2)
    {
        prop = do__get_var_e(query, name_nd, e);
        if (!prop)
            return e->nr;

        if (ti_opr_a_to_b(prop->val, tokens_nd, &query->rval, e))
            return e->nr;

        ti_val_drop(prop->val);
        prop->val = query->rval;
        ti_incref(prop->val);
        return e->nr;
    }

    /*
     * We must make variable assignable because we require a copy and need to
     * convert for example a tuple to a list.
     *
     * Closures can be ignored here because they are not mutable and will
     * unbound from the query if the `variable` is assigned or pushed to a list
     */
    if (    !ti_val_is_closure(query->rval) &&
            ti_val_make_variable(&query->rval, e))
        goto failed;

    name = name_nd->data
            ? name_nd->data
            : do__ensure_name_cache(query, name_nd);
    if (!name)
        goto alloc_err;

    prop = do__prop_scope(query, name);
    if (prop)
    {
        ti_val_drop(prop->val);
        prop->val = query->rval;
        ti_incref(prop->val);
        return e->nr;
    }

    ti_incref(name);
    prop = ti_prop_create(name, query->rval);
    if (!prop)
        goto alloc_err;

    if (vec_push(&query->vars, prop))
        goto alloc_err_with_prop;

    ti_incref(query->rval);
    return e->nr;

alloc_err_with_prop:
    /* prop->name will be dropped and prop->val is still on query->rval */
    free(prop);

alloc_err:
    ex_set_mem(e);

failed:
    ti_name_drop(name);
    return e->nr;
}

static inline int do__template(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (!nd->data)
    {
        if (ti_template_build(nd))
        {
            ex_set_mem(e);
            return e->nr;
        }
        assert (vec_space(query->val_cache));
        VEC_push(query->val_cache, nd->data);
    }
    return ti_template_compile(nd->data, query, e);
}

int ti_do_expression(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int preopr = (int) ((intptr_t) nd->children->node->data);
    cleri_children_t * child = nd               /* sequence */
            ->children                          /* first child, not */
            ->next;

    nd = child->node;                   /* immutable, function,
                                           assignment, name, thing,
                                           array, compare, closure */

    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        if (!query->collection)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "the `root` of the `%s` scope is inaccessible; "
                    "you might want to query a `@collection` scope?",
                    ti_query_scope_name(query));
            return e->nr;
        }

        query->rval = (ti_val_t *) query->collection->root;
        ti_incref(query->rval);

        if (do__chain(query, nd, e))
            return e->nr;

        /* nothing is possible after a chain */
        goto preopr;
    case CLERI_GID_THING_BY_ID:
        if (do__thing_by_id(query, nd, e))
            return e->nr;
        break;
    case CLERI_GID_T_CLOSURE:
        if (do__read_closure(query, nd, e))
            return e->nr;
        break;
    case CLERI_GID_T_FALSE:
        query->rval = (ti_val_t *) ti_vbool_get(false);
        break;
    case CLERI_GID_T_FLOAT:
        if (!nd->data)
        {
            nd->data = ti_vfloat_create(strx_to_double(nd->str));
            if (!nd->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, nd->data);
        }
        query->rval = nd->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_INT:
        if (!nd->data)
        {
            int64_t i = strx_to_int64(nd->str);
            if (errno == ERANGE)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            nd->data = ti_vint_create(i);
            if (!nd->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, nd->data);
        }
        query->rval = nd->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_NIL:
        query->rval = (ti_val_t *) ti_nil_get();
        break;
    case CLERI_GID_T_REGEX:
        if (!nd->data)
        {
            nd->data = ti_regex_from_strn(nd->str, nd->len, e);
            if (!nd->data)
                return e->nr;
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, nd->data);
        }
        query->rval = nd->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_STRING:
        if (!nd->data)
        {
            nd->data = ti_str_from_ti_string(nd->str, nd->len);
            if (!nd->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, nd->data);
        }
        query->rval = nd->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_TRUE:
        query->rval = (ti_val_t *) ti_vbool_get(true);
        break;
    case CLERI_GID_TEMPLATE:
        if (do__template(query, nd, e))
            return e->nr;
        break;
    case CLERI_GID_VAR_OPT_MORE:
        if (!nd->children->next)
        {
            if (do__var(query, nd->children->node, e))
                return e->nr;
            break;
        }

        switch (nd->children->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            if (do__function(query, nd, e))
                return e->nr;
            break;
        case CLERI_GID_ASSIGN:
            if (do__var_assign(query, nd, e))
                return e->nr;

            /* nothing is possible after assign since it ends with a scope */
            goto preopr;
        case CLERI_GID_INSTANCE:
            if (do__instance(query, nd, e))
                return e->nr;
            break;
        case CLERI_GID_ENUM_:
            if (do__enum(query, nd, e))
                return e->nr;
            break;
        default:
            assert (0);
            return -1;
        }
        break;
    case CLERI_GID_THING:
        if (do__thing(query, nd, e))
            return e->nr;
        break;
    case CLERI_GID_ARRAY:
        if (do__array(query, nd, e))
            return e->nr;
        break;
    case CLERI_GID_BLOCK:
        if (do__block(query, nd, e))
            return e->nr;
        break;
    case CLERI_GID_PARENTHESIS:
        if (ti_do_statement(query, nd->children->next->node, e))
            return e->nr;
        break;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    nd = child->node;

    /* handle index */
    if (do__index(query, nd, e))
        return e->nr;

    /* chain */
    if (child->next && do__chain(query, child->next->node, e))
        return e->nr;

preopr:
    return preopr ? ti_preopr_calc(preopr, &query->rval, e) : e->nr;
}
