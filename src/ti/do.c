/*
 * ti/do.c
 */
#include <cleri/cleri.h>
#include <doc.h>
#include <langdef/langdef.h>
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
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}

static inline ti_name_t * do__cache_name(ti_query_t * query, cleri_node_t * nd)
{
    assert (nd->data == NULL);
    ti_name_t * name = nd->data = ti_names_weak_get_strn(nd->str, nd->len);
    if (name)
    {
        ti_incref(name);
        VEC_push(query->immutable_cache, name);
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
    ti_type_t * type = ti_thing_type(thing);
    ti_name_t * name = nd->data ? nd->data : do__cache_name(query, nd);

    if (name)
    {
        ti_field_t * field;
        ti_method_t * method;

        if ((field = ti_field_by_name(type, name)))
        {
            wprop->name = name;
            wprop->val = (ti_val_t **) vec_get_addr(
                    thing->items.vec,
                    field->idx);
            return 0;
        }

        if ((method = ti_method_by_name(type, name)))
        {
            wprop->name = name;
            wprop->val = (ti_val_t **) (&method->closure);
            return 0;
        }
    }

    wprop->name = NULL;
    wprop->val = NULL;

    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no property or method `%.*s`",
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
    ti_name_t * name = name_nd->data
            ? name_nd->data
            : do__cache_name(query, name_nd);

    if (name && (field = ti_field_by_name(type, name)))
    {
        wprop->name = field->name;
        wprop->val = (ti_val_t **) vec_get_addr(
                thing->items.vec,
                field->idx);

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

    ti_val_unsafe_gc_drop(*wprop->val);
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
        task = ti_task_get_task(query->ev, thing);
        if (!task || ti_task_add_set(task, (ti_raw_t *) wprop.name, *wprop.val))
            ex_set_mem(e);
    }

done:
    ti_val_unlock((ti_val_t *) thing, true /* lock_was_set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

/*
 * Get variable by name from cache, or if the name is not in cache, cache
 * the property name.
 */
static inline ti_prop_t * do__get_var(ti_query_t * query, cleri_node_t * nd)
{
    return nd->data
            ? ti_query_var_get(query, nd->data)
            : (do__cache_name(query, nd)
                ? ti_query_var_get(query, nd->data)
                : NULL);
}

/*
 * Get variable or set an error message if the variable is not found.
 */
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

/*
 * Call a Type. Creates a new, empty, instance without arguments, returns
 * an existing instance if an ID is given, or a new instance based on a
 * given thing when a thing is given as argument.
 */
static int do__get_type_instance(
        ti_type_t * type,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (ti_type_wrap_only_e(type, e))
        return e->nr;

    if (nargs == 1)
    {
        int64_t id;
        ti_thing_t * thing;
        int lock_was_set = ti_type_ensure_lock(type);

        (void) ti_do_statement(query, nd->children->node, e);

        ti_type_unlock(type, lock_was_set);

        if (e->nr)
            return e->nr;

        if (!ti_val_is_int(query->rval))
        {
            if (!ti_val_is_thing(query->rval))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "cannot convert type `%s` to `%s`",
                        ti_val_str(query->rval),
                        type->name);
                return e->nr;
            }

            thing = ti_type_from_thing(type, (ti_thing_t *) query->rval, e);

            ti_val_unsafe_drop(query->rval);  /* from_thing */
            query->rval = (ti_val_t *) thing;

            return e->nr;
        }

        id = VINT(query->rval);
        thing = ti_query_thing_from_id(query, id, e);
        if (!thing)
            return e->nr;

        if (thing->type_id != type->type_id)
        {
            ex_set(e, EX_TYPE_ERROR,
                    TI_THING_ID" is of type `%s`, not `%s`",
                    thing->id, ti_val_str((ti_val_t *) thing), type->name);
            ti_decref(thing);
            return e->nr;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) thing;

        return 0;
    }

    if (nargs > 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
            "type `%s` takes at most 1 argument but %d were given",
            type->name,
            nargs);
        return e->nr;
    }

    query->rval = ti_type_dval(type);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}

static int do__get_enum_member(
        ti_enum_t * enum_,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (nargs == 1)
    {
        ti_member_t * member;
        int lock_was_set = ti_enum_ensure_lock(enum_);

        (void) ti_do_statement(query, nd->children->node, e);

        ti_enum_unlock(enum_, lock_was_set);

        if (e->nr)
            return e->nr;

        member = ti_enum_member_by_val_e(enum_, query->rval, e);
        if (!member)
            return e->nr;

        ti_incref(member);

        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) member;

        return e->nr;
    }

    if (nargs > 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
            "enum `%s` takes at most 1 argument but %d were given",
            enum_->name,
            nargs);
        return e->nr;
    }

    query->rval = ti_enum_dval(enum_);
    return e->nr;
}


static int do__function_call(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_node_t * fname = nd       /* sequence */
            ->children->node;       /* name node */

    cleri_node_t * args = nd        /* sequence */
        ->children->next->node      /* function sequence */
        ->children->next->node;     /* arguments */

    ti_prop_t * prop;

    /*
     * If `rval` is set, this means it is a "chained" function call,
     * for example:  my_thing.func()
     * This means we should check if `rval` is a thing or wrapped thing and
     * then see if the function exist on this thing.
     */
    if (query->rval)
        return fn_call_try_n(fname->str, fname->len, query, args, e);

    /*
     * When querying a collection, it is possible that the function call
     * is an attempt to initialize a type of enum.
     */
    if (query->collection)
    {
        ti_enum_t * enum_;
        ti_type_t * type;

        /* Try Type first as this is probably more common */
        type = ti_types_by_strn(
                query->collection->types,
                fname->str,
                fname->len);
        if (type)
            return do__get_type_instance(type, query, args, e);

        /* Try enum, there is never an overlap with Type since they are
         * unique with respect to each other
         */
        enum_ = ti_enums_by_strn(
                    query->collection->enums,
                    fname->str,
                    fname->len);
        if (enum_)
            return do__get_enum_member(enum_, query, args, e);
    }

    /*
     * If the call is not a type of enum, let's try if the function is a
     * variable of type closure which can be called. Props with build-in
     * function names, and/or exist as type/enum names are not reached and
     * can not be called in this way. It is possible to call these variable
     * using the `call` function: variable.call(...)
     */
    prop = do__get_var(query, fname);
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

    /*
     * It is not a build-in function, not a type or enum, and not a variable
     * so the function must be undefined.
     */
    ex_set(e, EX_LOOKUP_ERROR,
            "function `%.*s` is undefined",
            fname->len,
            fname->str);

    return e->nr;
}

static inline int do__function(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->children->next->node->cl_obj->gid == CLERI_GID_FUNCTION);
    /*
     * "Node -> data" is set for all build-in functions so they are preferred
     * over other functions/type/enum.
     */
    return nd->data
            ? ((fn_cb) nd->data)(
                    query,
                    nd                              /* sequence */
                        ->children->next->node      /* function sequence */
                        ->children->next->node,     /* arguments */
                    e)
            : do__function_call(query, nd, e);
}

static inline int do__block(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_BLOCK);

    cleri_children_t * child= nd->children->next->next
            ->node->children;  /* first child, not empty */

    do
    {
        if (ti_do_statement(query, child->node, e) ||
            !child->next ||
            !(child = child->next->next))
            break;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }
    while (1);

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
        ti_val_unsafe_drop((ti_val_t *) thing);
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

        ti_val_unsafe_drop(a);
        return e->nr;
    }
    case CLERI_GID_OPR6_CMP_AND:
        if (    ti_do_statement(query, nd->children->node, e) ||
                !ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
        return ti_do_statement(query, nd->children->next->next->node, e);
    case CLERI_GID_OPR7_CMP_OR:
        if (    ti_do_statement(query, nd->children->node, e) ||
                ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
        return ti_do_statement(query, nd->children->next->next->node, e);
    case CLERI_GID_OPR8_TERNARY:
        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        nd = ti_val_as_bool(query->rval)
                ? nd->children->next->node->children->next->node
                : nd->children->next->next->node;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        return ti_do_statement(query, nd, e);
    }

    assert (0);
    return e->nr;
}

static inline int do__thing_by_id(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    /*
     * Set node -> data to the actual thing when this is a normal query but
     * when used in a stored procedure, the `int` value is stored instead;
     * This syntax is probably not used frequently so do not worry a lot about
     * performance here; (and this is already pretty fast anyway...)
     */
    intptr_t thing_id = (intptr_t) nd->data;
    query->rval = (ti_val_t *) ti_query_thing_from_id(query, thing_id, e);
    return e->nr;
}

static int do__read_closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (!nd->data)
    {
        nd->data = (ti_val_t *) ti_closure_from_node(
                nd,
                (query->qbind.flags & TI_QBIND_FLAG_THINGSDB)
                    ? TI_CLOSURE_FLAG_BTSCOPE
                    : TI_CLOSURE_FLAG_BCSCOPE);
        if (!nd->data)
        {
            ex_set_mem(e);
            return e->nr;
        }
        assert (vec_space(query->immutable_cache));
        VEC_push(query->immutable_cache, nd->data);
    }
    query->rval = nd->data;
    ti_incref(query->rval);

    return e->nr;
}

/*
 * Use the command below to generate a perfect hash for the fixed mapping
 * constants. Fixed constants are available in non-collection scopes only.
 * This allows to find the constants in the fastest possible way.
 *
 *   pcregrep -o1 '\.name\=\"(\w+)' do.c | gperf -E -k '*,1,$' -m 200
 */

enum
{
    TOTAL_KEYWORDS = 13,
    MIN_WORD_LENGTH = 3,
    MAX_WORD_LENGTH = 8,
    MIN_HASH_VALUE = 4,
    MAX_HASH_VALUE = 16
};

static inline unsigned int do__hash(
        register const char * s,
        register size_t n)
{
    static unsigned char asso_values[] =
    {
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17,  1,  5,  1,  5,  0,
         2,  1,  6,  0, 17, 17,  3,  0,  1,  2,
        17,  0,  0, 17,  0,  0,  0,  0, 17,  0,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17
    };

    register unsigned int hval = n;

    switch (hval)
    {
        default:
            hval += asso_values[(unsigned char)s[7]];
            /*fall through*/
        case 7:
            hval += asso_values[(unsigned char)s[6]];
            /*fall through*/
        case 6:
            hval += asso_values[(unsigned char)s[5]];
            /*fall through*/
        case 5:
            hval += asso_values[(unsigned char)s[4]];
            /*fall through*/
        case 4:
            hval += asso_values[(unsigned char)s[3]];
            /*fall through*/
        case 3:
            hval += asso_values[(unsigned char)s[2]];
            /*fall through*/
        case 2:
            hval += asso_values[(unsigned char)s[1]];
            /*fall through*/
        case 1:
            hval += asso_values[(unsigned char)s[0]];
            break;
    }

    return hval;
}


typedef struct
{
    char name[MAX_WORD_LENGTH+1];
    int value;
    ti_val_t * val;
    size_t n;
} do__fixed_t;

do__fixed_t do__fixed_mapping[TOTAL_KEYWORDS] = {
    {.name="READ",                  .value=TI_AUTH_QUERY},  /* deprecated */
    {.name="QUERY",                 .value=TI_AUTH_QUERY},
    {.name="MODIFY",                .value=TI_AUTH_EVENT},  /* deprecated */
    {.name="EVENT",                 .value=TI_AUTH_EVENT},
    {.name="WATCH",                 .value=TI_AUTH_WATCH},
    {.name="RUN",                   .value=TI_AUTH_RUN},
    {.name="GRANT",                 .value=TI_AUTH_GRANT},
    {.name="FULL",                  .value=TI_AUTH_MASK_FULL},
    {.name="DEBUG",                 .value=LOGGER_DEBUG},
    {.name="INFO",                  .value=LOGGER_INFO},
    {.name="WARNING",               .value=LOGGER_WARNING},
    {.name="ERROR",                 .value=LOGGER_ERROR},
    {.name="CRITICAL",              .value=LOGGER_CRITICAL},
};

static do__fixed_t * do__fixed_map[MAX_HASH_VALUE+1];

int ti_do_init(void)
{
    /*
     * This code only runs once at startup and prepares perfect hashing for
     * constant variable names;
     */
    for (size_t i = 0, n = TOTAL_KEYWORDS; i < n; ++i)
    {
        uint32_t key;
        do__fixed_t * fixed = &do__fixed_mapping[i];

        fixed->n = strlen(fixed->name);
        fixed->val = (ti_val_t *) ti_vint_create(fixed->value);
        if (!fixed->val)
        {
            ti_do_drop();
            return -1;
        }

        key = do__hash(fixed->name, fixed->n);

        assert (fixed->val);
        assert (do__fixed_map[key] == NULL);
        assert (key <= MAX_HASH_VALUE);

        do__fixed_map[key] = fixed;
    }
    return 0;
}

void ti_do_drop(void)
{
    for (size_t i = 0, n = TOTAL_KEYWORDS; i < n; ++i)
    {
        do__fixed_t * fixed = &do__fixed_mapping[i];
        ti_val_drop(fixed->val);  /* might be uninitialized at early stop */
    }
}

static int do__fixed_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /*
     * This function is only called in a non-collection scope as all known
     * constants are only applicable in the node- and thingsdb scope.
     */
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_VAR);

    register size_t n = nd->len;
    register uint32_t key = do__hash(nd->str, n);
    register do__fixed_t * fixed = key <= MAX_HASH_VALUE
            ? do__fixed_map[key]
            : NULL;

    if (fixed && fixed->n == n && memcmp(fixed->name, nd->str, n) == 0)
    {
        query->rval = fixed->val;
        ti_incref(query->rval);
        return 0;
    }

    ex_set(e, EX_LOOKUP_ERROR,
            "variable `%.*s` is undefined",
            (int) nd->len, nd->str);

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
                !ti_thing_p_prop_set(thing, name, query->rval))
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
    ti_val_unsafe_drop((ti_val_t *) thing);
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

    if (ti_type_wrap_only_e(type, e))
        return e->nr;

    thing = ti_thing_t_create(0, type, query->collection);
    if (!thing)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (n = type->fields->n; n--;)
        VEC_push(thing->items.vec, NULL);

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

        name = ti_names_weak_get_strn(name_nd->str, name_nd->len);
        if (!name)
            continue;

        field = ti_field_by_name(type, name);
        if (!field)
            continue;

        if (    ti_do_statement(query, scope, e) ||
                ti_field_make_assignable(field, &query->rval, thing, e))
            goto fail;

        val = VEC_get(thing->items.vec, field->idx);
        if (val)
            ti_val_unsafe_gc_drop(val);
        else
            ++n;
        vec_set(thing->items.vec, query->rval, field->idx);

        query->rval = NULL;
    }

    if (n < type->fields->n)
    {
        /* fill missing fields */
        for (vec_each(type->fields, ti_field_t, field))
        {
            if (!VEC_get(thing->items.vec, field->idx))
            {
                ti_val_t * val = field->dval_cb(field);
                if (!val)
                {
                    ex_set_mem(e);
                    goto fail;
                }

                ti_val_attach(val, thing, field);
                vec_set(thing->items.vec, val, field->idx);
            }
        }
    }

    query->rval = (ti_val_t *) thing;
    goto done;

fail:
    ti_val_unsafe_drop((ti_val_t *) thing);

done:
    ti_type_unlock(type, lock_was_set);
    return e->nr;
}

static int do__enum_get(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_enum_t * enum_;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    nd = nd                                     /* sequence */
            ->children->next->node              /* enum node */
            ->children->next->node;             /* name or closure */

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
        query->rval = (ti_val_t *) ti_enum_member_by_strn(
                enum_,
                nd->str,
                nd->len);
        if (query->rval)
            ti_incref(query->rval);
        else
            ex_set(e, EX_LOOKUP_ERROR, "enum `%s` has no member `%.*s`",
                    enum_->name,
                    nd->len,
                    nd->str);
        return e->nr;
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
        ti_val_unsafe_drop((ti_val_t *) closure);
        vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);

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

        ti_val_unsafe_drop((ti_val_t *) rname);
        break;
    }
    }

    return e->nr;
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
            /*
             * Look for fixed constants only if this is not a collection scope.
             */
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
    register uint32_t end = query->local_stack;

    /*
     * End is the position in the variable stack where this body has
     * started, therefore we start at the end and then look back until
     * the "end" position in the stack.
     */
    while (n-- > end)
    {
        ti_prop_t * prop = VEC_get(query->vars, n);
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
    /*
     * Function `ti_qbind_probe(..)` has checked how much cache must be
     * available, therefore we can use `VEC_push` there must be enough space
     * to fit this name.
     */
    if (name)
        VEC_push(query->immutable_cache, name);
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

    /*
     * When `tokens_nd` has a length of 2, this means the statement wants to
     * update an existing variable using something like +=, -= etc.
     * The only other option is a token length of 1 which is an assignment
     * using token `=`.
     */
    if (tokens_nd->len == 2)
    {
        /*
         * Since the statement attempts to update an existing variable, the
         * prop must exist.
         */
        prop = do__get_var_e(query, name_nd, e);
        if (!prop)
            return e->nr;

        /* update value `a` with value `b` but store the value in `b` */
        if (ti_opr_a_to_b(prop->val, tokens_nd, &query->rval, e))
            return e->nr;

        /* switch `a` with `b` as the new value is set to `b` */
        ti_val_unsafe_drop(prop->val);
        prop->val = query->rval;
        ti_incref(prop->val);
        return e->nr;
    }

    /*
     * Must make variable assignable because we require a copy and need to
     * convert for example a tuple to a list.
     *
     * Closures can be ignored here because they are not mutable and will
     * unbound from the query if the `variable` is assigned or pushed to a list
     */
    if (    !ti_val_is_closure(query->rval) &&
            ti_val_make_variable(&query->rval, e))
        goto failed;

    /*
     * Try to get the name from cache, else ensure it is set on the cache for
     * the next time the code visits this same point.
     */
    name = name_nd->data
            ? name_nd->data
            : do__ensure_name_cache(query, name_nd);
    if (!name)
        goto alloc_err;

    /*
     * Check if the `prop` already is available in this scope on the
     * stack, and if * this is the case, then update the `prop` value with the
     * new value and return.
     */
    prop = do__prop_scope(query, name);
    if (prop)
    {
        ti_val_unsafe_gc_drop(prop->val);
        prop->val = query->rval;
        ti_incref(prop->val);
        return e->nr;
    }

    /*
     * Create a new `prop` and store the `prop` in this scope on the
     * stack. Only allocation errors might screw things up.
     */
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
    /*
     * The template will be build and stored on node -> data the first time
     * it reaches this code. Function `ti_qbind_probe(..)` has to make sure
     * there is enough room on the cache to store a pointer to the build
     * template.
     */
    if (!nd->data)
    {
        /*
         * This will build a template which can be re-used with different
         * values and rather quickly may generate new strings based on the
         * template and values.
         */
        if (ti_template_build(nd))
        {
            ex_set_mem(e);
            return e->nr;
        }
        assert (vec_space(query->immutable_cache));
        VEC_push(query->immutable_cache, nd->data);
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
        /*
         * When an expression starts with a dot, this is always the root
         * of the collection; For example .id(); or .prop;
         */
        if (!query->collection)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "the `root` of the `%s` scope is inaccessible; "
                    "you might want to query a `@collection` scope?",
                    ti_query_scope_name(query));
            return e->nr;
        }

        /* Set the collection at the current value */
        query->rval = (ti_val_t *) query->collection->root;
        ti_incref(query->rval);

        if (do__chain(query, nd, e))
            return e->nr;

        /* nothing is possible after a chain */
        goto preopr;
    case CLERI_GID_THING_BY_ID:
        /*
         * Get a thing by ID using the hash (#) syntax, for example: #123;
         */
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
            nd->data = ti_vfloat_create(strx_to_double(nd->str, NULL));
            if (!nd->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            assert (vec_space(query->immutable_cache));
            VEC_push(query->immutable_cache, nd->data);
        }
        query->rval = nd->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_INT:
        if (!nd->data)
        {
            int64_t i = strx_to_int64(nd->str, NULL);
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
            assert (vec_space(query->immutable_cache));
            VEC_push(query->immutable_cache, nd->data);
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
            assert (vec_space(query->immutable_cache));
            VEC_push(query->immutable_cache, nd->data);
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
            assert (vec_space(query->immutable_cache));
            VEC_push(query->immutable_cache, nd->data);
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
            if (do__enum_get(query, nd, e))
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
