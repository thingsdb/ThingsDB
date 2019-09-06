/*
 * ti/do.c
 */
#include <ti/auth.h>
#include <ti/do.h>
#include <ti/regex.h>
#include <ti/vint.h>
#include <ti/task.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr/oprinc.h>
#include <cleri/cleri.h>
#include <langdef/nd.h>
#include <langdef/langdef.h>
#include <util/strx.h>

#define SLICES_DOC_ TI_SEE_DOC("#slices")

static inline int do__no_collection_scope(ti_query_t * query)
{
    return ~query->syntax.flags & TI_SYNTAX_FLAG_COLLECTION;
}

typedef int (*do__fn_cb) (ti_query_t *, cleri_node_t *, ex_t *);

static inline int do__function(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_FUNCTION);

    cleri_node_t * fname = nd       /* sequence */
            ->children->node;       /* name node */

    if (!fname->data)
    {
        if (query->rval)
            ex_set(e, EX_INDEX_ERROR,
                    "type `%s` has no function `%.*s`",
                    ti_val_str(query->rval),
                    fname->len,
                    fname->str);
        else
            ex_set(e, EX_INDEX_ERROR,
                    "function `%.*s` is undefined",
                    fname->len,
                    fname->str);
        return e->nr;
    }

    return ((do__fn_cb) fname->data)(query, nd->children->next->next->node, e);
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

    if (query->target && sz >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached"
                TI_SEE_DOC("#quotas"), query->target->quota->max_array_size);
        return e->nr;
    }

    varr = ti_varr_create(sz);
    if (!varr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (; child; child = child->next->next)
    {
        if (ti_do_scope(query, child->node, e))
            goto failed;

        if (ti_varr_append(varr, (void **) &query->rval, e))
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

static inline ti_prop_t * do__get_prop(
        ti_query_t * query,
        ti_thing_t * thing,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_prop_t * prop = nd->data
            ? ti_thing_prop_weak_get(thing, nd->data)
            : (do__cache_name(query, nd)
                ? ti_thing_prop_weak_get(thing, nd->data)
                : NULL);

    if (!prop)
        ex_set(e, EX_INDEX_ERROR,
                "thing "TI_THING_ID" has no property `%.*s`",
                thing->id, (int) nd->len, nd->str);

    return prop;
}

static int do__assignment(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);
    assert (query->rval);

    ti_thing_t * thing;
    ti_task_t * task;
    ti_prop_t * prop;
    size_t max_props = query->target
            ? query->target->quota->max_props
            : TI_QUOTA_NOT_SET;     /* check for target since assign is
                                       possible when chained in all scopes */
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, scope_nd, e))
        goto done;

    if (assign_nd->len == 2)
    {
        prop = do__get_prop(query, thing, name_nd, e);
        if (!prop)
            goto done;

        if (ti_opr_a_to_b(prop->val, assign_nd, &query->rval, e))
            goto done;

        ti_val_drop(prop->val);
        prop->val = query->rval;
    }
    else
    {
        ti_name_t * name;

        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                "maximum properties quota of %zu has been reached"
                TI_SEE_DOC("#quotas"), max_props);
            goto done;
        }

        if (ti_val_make_assignable(&query->rval, e))
            goto done;

        name = ti_names_get(name_nd->str, name_nd->len);
        if (!name)
            goto alloc_err;

        prop = ti_thing_prop_set_e(thing, name, query->rval, e);
        if (!prop)
        {
            assert (e->nr);
            ti_name_drop(name);
            goto done;
        }
    }
    ti_incref(prop->val);

    if (thing->id)
    {
        assert (query->target);  /* only in a collection scope */
        task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto done;

        if (ti_task_add_set(task, prop->name, prop->val))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    goto done;

alloc_err:
    ex_set_mem(e);

done:
    ti_val_unlock((ti_val_t *) thing, true /* lock_was_set */);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__block(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_BLOCK);

    cleri_children_t * child, * seqchild;

    uint32_t current_varn = query->vars->n;

    seqchild = nd                       /* <{ comment, list s }> */
        ->children->next->next;         /* list statements */

    child = seqchild->node->children;  /* first child, not empty */

    assert (child);

    while (1)
    {
        if (ti_do_scope(query, child->node, e))
            return e->nr;

        if (!child->next || !(child = child->next->next))
            break;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    while (query->vars->n > current_varn)
        ti_prop_destroy(vec_pop(query->vars));

    return e->nr;
}

static inline size_t do__slice_also_not_nil(ti_val_t * val, ex_t * e)
{
    if (!ti_val_is_nil(val))
        ex_set(e, EX_BAD_DATA,
               "slice indices must be of type `"TI_VAL_INT_S"` "
               "or `"TI_VAL_NIL_S"` but got got type `%s` instead"
               SLICES_DOC_, ti_val_str(val));
    return e->nr;
}

static int do__read_slice_indices(
        ti_query_t * query,
        cleri_node_t * slice,
        ssize_t * start,
        ssize_t * stop,
        ssize_t * step,
        ex_t * e)
{
    assert (query->rval == NULL);

    const ssize_t n = *stop;
    cleri_children_t * child = slice->children;
    ssize_t * start_ = NULL;
    ssize_t * stop_ = NULL;

    if (!child || !(*stop))
        return e->nr;

    if (child->node->cl_obj->gid == CLERI_GID_SCOPE)
    {
        if (ti_do_scope(query, child->node, e))
            return e->nr;

        if (ti_val_is_int(query->rval))
        {
            ssize_t i = ((ti_vint_t *) query->rval)->int_;
            if (i > 0)
            {
                *start = i >= n ? n - 1 : i;
            }
            else if (i < 0 && (i = n + i) > 0)
            {
                *start = i;
            }
            start_ = start;
        }
        else if (do__slice_also_not_nil(query->rval, e))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;

        child = child->next;
    }

    assert (child);  /* ':' */

    child = child->next;
    if (!child)
        return e->nr;

    if (child->node->cl_obj->gid == CLERI_GID_SCOPE)
    {
        if (ti_do_scope(query, child->node, e))
            return e->nr;

        if (ti_val_is_int(query->rval))
        {
            ssize_t i = ((ti_vint_t *) query->rval)->int_;
            if (i < n)
            {
                *stop = i < 0 ? ((n + i) < 0 ? 0 : n + i) : i;
            }
            stop_ = stop;
        }
        else if (do__slice_also_not_nil(query->rval, e))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;

        child = child->next;
        if (!child)
            return e->nr;
    }

    assert (child);  /* ':' */

    child = child->next;

    if (child)  /* must be scope since no more indices are allowed */
    {
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_do_scope(query, child->node, e))
            return e->nr;

        if (ti_val_is_int(query->rval))
        {
            ssize_t i = ((ti_vint_t *) query->rval)->int_;
            if (i < 0)
            {
                if (!start_)
                    *start = n -1;
                if (!stop_)
                    *stop = -1;
            }
            *step = i;
        }
        else if (do__slice_also_not_nil(query->rval, e))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    return e->nr;
}

static int do__slice_raw(ti_query_t * query, cleri_node_t * slice, ex_t * e)
{
    ti_raw_t * source = (ti_raw_t *) query->rval;
    ssize_t start = 0, stop = (ssize_t) source->n, step = 1;

    query->rval = NULL;

    if (do__read_slice_indices(query, slice, &start, &stop, &step, e))
        goto done;

    query->rval = (ti_val_t *) ti_raw_from_slice(source, start, stop, step);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int do__slice_arr(ti_query_t * query, cleri_node_t * slice, ex_t * e)
{
    ti_varr_t * source = (ti_varr_t *) query->rval;
    ssize_t start = 0, stop = (ssize_t) source->vec->n, step = 1;

    query->rval = NULL;

    if (do__read_slice_indices(query, slice, &start, &stop, &step, e))
        goto done;

    query->rval = (ti_val_t *) ti_varr_from_slice(source, start, stop, step);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int do__slice_arr_a(ti_query_t * query, cleri_node_t * slice, ex_t * e)
{
    return 0;
}

static int do__index_numeric(
        ti_query_t * query,
        cleri_node_t * scope,
        size_t * idx,
        size_t n,
        ex_t * e)
{
    ssize_t i;

    if (ti_do_scope(query, scope, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "expecting an index of type `"TI_VAL_INT_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    i = ((ti_vint_t * ) query->rval)->int_;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (i < 0)
        i += n;

    if (i < 0 || i >= (ssize_t) n)
        ex_set(e, EX_INDEX_ERROR, "index out of range");
    else
        *idx = (size_t) i;

    return e->nr;
}

static int do__index_raw(ti_query_t * query, cleri_node_t * scope, ex_t * e)
{
    ti_raw_t * source = (ti_raw_t *) query->rval;
    size_t idx;

    query->rval = NULL;

    if (do__index_numeric(query, scope, &idx, source->n, e))
        goto done;

    query->rval = (ti_val_t *) ti_raw_create(source->data + idx, 1);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int do__index_arr(ti_query_t * query, cleri_node_t * scope, ex_t * e)
{
    ti_varr_t * source = (ti_varr_t *) query->rval;
    size_t idx;

    query->rval = NULL;

    if (do__index_numeric(query, scope, &idx, source->vec->n, e))
        goto done;

    query->rval = vec_get(source->vec, idx);
    ti_incref(query->rval);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int do__index_arr_a(ti_query_t * query, cleri_node_t * index, ex_t * e)
{
    cleri_node_t * idx_scope = index->children->next->node->children->node;
    cleri_node_t * ass_scope = index->children->next->next->next->node
            ->children->next->node;
    ti_varr_t * varr;
    ti_chain_t chain;
    size_t idx;

    ti_chain_move(&chain, &query->chain);

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (do__index_numeric(query, idx_scope, &idx, varr->vec->n, e))
        goto fail1;

    if (ti_do_scope(query, ass_scope, e))
        goto fail1;

    ti_val_drop((ti_val_t *) vec_get(varr->vec, idx));

    varr->vec->data[idx] = query->rval;
    ti_incref(query->rval);

    if (ti_chain_is_set(&chain))
    {
        ti_task_t * task = ti_task_get_task(query->ev, chain.thing, e);
        if (!task)
            goto fail1;

        if (ti_task_add_splice(
                task,
                chain.name,
                varr,
                idx,
                1,
                1))
            ex_set_mem(e);
    }

fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_drop((ti_val_t *) varr);
fail0:
    ti_chain_unset(&chain);
    return e->nr;
}

static int do__single_index(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->rval);

    ti_val_t * val = query->rval;
    cleri_node_t * slice = nd->children->next->node;

    assert (slice->cl_obj->gid == CLERI_GID_SLICE);

    _Bool do_assign = !!nd->children->next->next->next;
    _Bool do_slice = (
            !slice->children ||
            slice->children->next ||
            slice->children->node->cl_obj->tp == CLERI_TP_TOKEN
    );

    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NAME:
    case TI_VAL_RAW:
        if (do_assign)
            goto assign_error;

        return do_slice
                ? do__slice_raw(query, slice, e)
                : do__index_raw(query, slice->children->node, e);

    case TI_VAL_ARR:
        if (do_assign && ti_varr_is_tuple((ti_varr_t *) val))
            goto assign_error;

        return do_assign
                ? do_slice
                    ? do__slice_arr_a(query, nd, e)
                    : do__index_arr_a(query, nd, e)
                : do_slice
                    ? do__slice_arr(query, slice, e)
                    : do__index_arr(query, slice->children->node, e);

    case TI_VAL_THING:
        if (do_slice)
            goto slice_error;

        break;
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_QP:
    case TI_VAL_REGEX:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        if (do_slice)
            goto slice_error;
        goto index_error;
    }

    assert (0);
    return e->nr;

slice_error:
    ex_set(e, EX_BAD_DATA, "type `%s` has no slice support",
            ti_val_str(val));
    return e->nr;

index_error:
    ex_set(e, EX_BAD_DATA, "type `%s` is not indexable",
            ti_val_str(val));
    return e->nr;

assign_error:
    ex_set(e, EX_BAD_DATA, "type `%s` does not support index assignments",
        ti_val_str(val));
    return e->nr;
}

static inline int do__index(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_children_t * child = nd->children;

    for (child = nd->children; child; child = child->next)
        if (do__single_index(query, child->node, e))
            return e->nr;
    return 0;
}

static int do__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);
    assert (query->rval);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */

    cleri_node_t * node = child->node;      /* function, assignment,
                                               name */
    cleri_node_t * index_node = child->next->node;

    child = child->next->next;          /* set to chain child (or NULL) */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        if (do__function(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (do__assignment(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
    {
        ti_prop_t * prop;
        ti_thing_t * thing;

        if (!ti_val_is_thing(query->rval))
        {
            ex_set(e, EX_BAD_DATA, "type `%s` has no properties",
                    ti_val_str(query->rval));
            return e->nr;
        }

        thing = (ti_thing_t *) query->rval;

        prop = do__get_prop(query, thing, node, e);
        if (!prop)
            return e->nr;

        if (thing->id && (index_node->children || child))
            ti_chain_set(&query->chain, thing, prop->name);

        query->rval = prop->val;
        ti_incref(query->rval);
        ti_val_drop((ti_val_t *) thing);

        break;
    }
    default:
        assert (0);  /* all possible should be handled */
        goto done;
    }

    if (do__index(query, index_node, e))
        goto done;

    if (child)
        (void) do__chain(query, child->node, e);

done:
    ti_chain_unset(&query->chain);
    return e->nr;
}

static int do__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    uint32_t gid;
    ti_val_t * a_val = NULL;
    assert( nd->cl_obj->tp == CLERI_TP_PRIO );

    nd = nd->children->node;        /* compare sequence */

    assert (nd->cl_obj->tp == CLERI_TP_SEQUENCE);
    assert (query->rval == NULL);

    if (nd->cl_obj->gid == CLERI_GID_SCOPE)
        return ti_do_scope(query, nd, e);

    gid = nd->children->next->node->cl_obj->gid;

    switch (gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_AND:
    case CLERI_GID_OPR3_BITWISE_XOR:
    case CLERI_GID_OPR4_BITWISE_OR:
    case CLERI_GID_OPR5_COMPARE:
        if (do__operations(query, nd->children->node, e))
            return e->nr;
        a_val = query->rval;
        query->rval = NULL;
        if (do__operations(query, nd->children->next->next->node, e))
            break;
        (void) ti_opr_a_to_b(a_val, nd->children->next->node, &query->rval, e);
        break;

    case CLERI_GID_OPR6_CMP_AND:
        if (    do__operations(query, nd->children->node, e) ||
                !ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return do__operations(query, nd->children->next->next->node, e);

    case CLERI_GID_OPR7_CMP_OR:
        if (    do__operations(query, nd->children->node, e) ||
                ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return do__operations(query, nd->children->next->next->node, e);

    default:
        assert (0);
    }

    ti_val_drop(a_val);
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

static int do__immutable(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_IMMUTABLE);
    assert (!e->nr);

    cleri_node_t * node = nd            /* choice */
            ->children->node;           /* false, nil, true, undefined,
                                           int, float, string */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_CLOSURE:
        if (!node->data)
        {

            node->data = (ti_val_t *) ti_closure_from_node(
                    node,
                    (query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB)
                        ? TI_VFLAG_CLOSURE_BTSCOPE
                        : TI_VFLAG_CLOSURE_BCSCOPE);
            if (!node->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_FALSE:
        query->rval = (ti_val_t *) ti_vbool_get(false);
        break;
    case CLERI_GID_T_FLOAT:
        if (!node->data)
        {
            node->data = ti_vfloat_create(strx_to_double(node->str));
            if (!node->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_INT:
        if (!node->data)
        {
            node->data = ti_vint_create(strx_to_int64(node->str));
            if (!node->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            if (errno == ERANGE)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_NIL:
        query->rval = (ti_val_t *) ti_nil_get();
        break;
    case CLERI_GID_T_REGEX:
        if (!node->data)
        {
            node->data = ti_regex_from_strn(node->str, node->len, e);
            if (!node->data)
                return e->nr;
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_STRING:
        if (!node->data)
        {
            node->data = ti_raw_from_ti_string(node->str, node->len);
            if (!node->data)
            {
                ex_set_mem(e);
                return e->nr;
            }
            assert (vec_space(query->val_cache));
            VEC_push(query->val_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_TRUE:
        query->rval = (ti_val_t *) ti_vbool_get(true);
        break;
    }
    assert (e->nr || query->rval);  /* rval must be set when e->nr == 0 */
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
        ex_set(e, EX_INDEX_ERROR,
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
    size_t max_props;

    if (query->target)
    {
        if (ti_quota_things(query->target->quota, query->target->things->n, e))
            return e->nr;
        max_props = query->target->quota->max_props;
        thing = ti_thing_create(0, query->target->things);
    }
    else
    {
        max_props = TI_QUOTA_NOT_SET;
        thing = ti_thing_create(0, NULL);
    }

    if (!thing)
        goto alloc_err;

    child = nd                                  /* sequence */
            ->children->next->node              /* list */
            ->children;                         /* list items */

    for (; child; child = child->next->next)
    {
        cleri_node_t * name_nd;
        cleri_node_t * scope;
        ti_name_t * name;

        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum properties quota of %zu has been reached"
                    TI_SEE_DOC("#quotas"), max_props);
            goto err;
        }

        name_nd = child->node                       /* sequence */
                ->children->node;                   /* name */

        scope = child->node                         /* sequence */
                ->children->next->next->node;       /* scope */

        if (    ti_do_scope(query, scope, e) ||
                ti_val_make_assignable(&query->rval, e))
            goto err;

        name = ti_names_get(name_nd->str, name_nd->len);
        if (!name)
            goto alloc_err;

        if (!ti_thing_prop_set(thing, name, query->rval))
        {
            ti_name_drop(name);
            goto alloc_err;
        }

        query->rval = NULL;

        if (!child->next)
            break;
    }

    query->rval = (ti_val_t *) thing;
    return 0;

alloc_err:
    ex_set_mem(e);
err:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static inline ti_prop_t * do__get_var(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_prop_t * prop = nd->data
            ? ti_query_var_get(query, nd->data)
            : (do__cache_name(query, nd)
                ? ti_query_var_get(query, nd->data)
                : NULL);

    if (!prop)
        ex_set(e, EX_INDEX_ERROR,
                "variable `%.*s` is undefined",
                (int) nd->len, nd->str);

    return prop;
}

/* changes scope->name and/or scope->thing */
static int do__var(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_VAR);
    assert (ti_name_is_valid_strn(nd->str, nd->len));
    assert (query->rval == NULL);
    assert (!ti_chain_is_set(&query->chain));

    ti_prop_t * prop = do__get_var(query, nd, e);

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

static int do__var_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_VAR_ASSIGN);
    assert (query->rval == NULL);
    assert (!ti_chain_is_set(&query->chain));

    ti_name_t * name = NULL;
    ti_prop_t * prop = NULL;     /* assign to prevent warning */
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */


    if (ti_do_scope(query, scope_nd, e))
        return e->nr;

    if (assign_nd->len == 2)
    {
        prop = do__get_var(query, name_nd, e);
        if (!prop)
            return e->nr;

        if (ti_opr_a_to_b(prop->val, assign_nd, &query->rval, e))
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
            ti_val_make_assignable(&query->rval, e))
        goto failed;

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

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

int ti_do_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);
    assert (query->rval == NULL);

    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd         /* sequence */
            ->children;                             /* first child, not */

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node;                 /* immutable, function,
                                           assignment, name, thing,
                                           array, compare, closure */

    ti_chain_unset(&query->chain);

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        if (do__array(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_CHAIN:
        if (!query->target)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "the `root` of the `%s` scope is inaccessible; "
                    "You might want to query a `collection` scope?",
                    ti_query_scope_name(query));
            return e->nr;
        }
        query->rval = (ti_val_t *) query->root;
        ti_incref(query->rval);
        if (do__chain(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_BLOCK:
        if (do__block(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_FUNCTION:
        if (do__function(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_IMMUTABLE:
        if (do__immutable(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (do__operations(query, node->children->next->node, e))
            return e->nr;

        if (node->children->next->next->next)   /* optional (sequence) */
        {
            _Bool bool_ = ti_val_as_bool(query->rval);
            ti_val_drop(query->rval);
            query->rval = NULL;
            node = node->children->next->next->next->node;  /* sequence */
            if (bool_)
            {
                if (ti_do_scope(query, node->children->next->node, e))
                    return e->nr;
            }
            else if (node->children->next->next) /* else case */
            {
                node = node->children->next->next->node;  /* else sequence */
                if (ti_do_scope(query, node->children->next->node, e))
                    return e->nr;
            }
            else  /* no else case, just nil */
            {
                query->rval = (ti_val_t *) ti_nil_get();
            }
        }
        break;
    case CLERI_GID_THING:
        if (do__thing(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_THING_BY_ID:
        if (do__thing_by_id(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_VAR:
        if (do__var(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_VAR_ASSIGN:
        if (do__var_assign(query, node, e))
            return e->nr;
        break;

    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    node = child->node;

    /* handle index */
    if (do__index(query, node, e))
        return e->nr;

    /* chain */
    if (child->next && do__chain(query, child->next->node, e))
        return e->nr;

    if (nots)
    {
        _Bool b = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_vbool_get((nots & 1) ^ b);
    }

    return e->nr;
}
