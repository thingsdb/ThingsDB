/*
 * ti/index.c
 */
#include <ti/index.h>
#include <ti/do.h>
#include <ti/vint.h>
#include <ti/task.h>
#include <ti/nil.h>
#include <ti/prop.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/opr/oprinc.h>
#include <langdef/langdef.h>

#define SLICES_DOC_ TI_SEE_DOC("#slices")

static inline size_t index__slice_also_not_nil(ti_val_t * val, ex_t * e)
{
    if (!ti_val_is_nil(val))
        ex_set(e, EX_BAD_DATA,
               "slice indices must be of type `"TI_VAL_INT_S"` "
               "or `"TI_VAL_NIL_S"` but got got type `%s` instead"
               SLICES_DOC_, ti_val_str(val));
    return e->nr;
}

static int index__read_slice_indices(
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

    if (child->node->cl_obj->gid == CLERI_GID_STATEMENT)
    {
        if (ti_do_statement(query, child->node, e))
            return e->nr;

        if (ti_val_is_int(query->rval))
        {
            ssize_t i = ((ti_vint_t *) query->rval)->int_;
            if (i > 0)
            {
                *start = i > n ? n : i;
            }
            else if (i < 0 && (i = n + i) > 0)
            {
                *start = i;
            }
            start_ = start;
        }
        else if (index__slice_also_not_nil(query->rval, e))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;

        child = child->next;
    }

    assert (child);  /* ':' */

    child = child->next;
    if (!child)
        return e->nr;

    if (child->node->cl_obj->gid == CLERI_GID_STATEMENT)
    {
        if (ti_do_statement(query, child->node, e))
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
        else if (index__slice_also_not_nil(query->rval, e))
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
        assert (child->node->cl_obj->gid == CLERI_GID_STATEMENT);

        if (ti_do_statement(query, child->node, e))
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
        else if (index__slice_also_not_nil(query->rval, e))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    return e->nr;
}

static int index__slice_raw(ti_query_t * query, cleri_node_t * slice, ex_t * e)
{
    ti_raw_t * source = (ti_raw_t *) query->rval;
    ssize_t start = 0, stop = (ssize_t) source->n, step = 1;

    query->rval = NULL;

    if (index__read_slice_indices(query, slice, &start, &stop, &step, e))
        goto done;

    query->rval = (ti_val_t *) ti_raw_from_slice(source, start, stop, step);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int index__slice_arr(ti_query_t * query, cleri_node_t * slice, ex_t * e)
{
    ti_varr_t * source = (ti_varr_t *) query->rval;
    ssize_t start = 0, stop = (ssize_t) source->vec->n, step = 1;

    query->rval = NULL;

    if (index__read_slice_indices(query, slice, &start, &stop, &step, e))
        goto done;

    query->rval = (ti_val_t *) ti_varr_from_slice(source, start, stop, step);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int index__slice_ass(ti_query_t * query, cleri_node_t * inode, ex_t * e)
{
    cleri_node_t * slice = inode->children->next->node;
    cleri_node_t * ass_scope = inode->children->next->next->next->node;
    cleri_node_t * ass_tokens = ass_scope->children->node;
    ti_varr_t * evarr, * varr = (ti_varr_t *) query->rval;
    ti_chain_t chain;
    ssize_t c, n;
    ssize_t start = 0, stop = (ssize_t) varr->vec->n, step = 1;
    uint32_t current_n, new_n;

    if (ass_tokens->len == 2)
    {
        ex_set(e, EX_BAD_DATA,
                "unsupported operand type `%.*s` for slice assignments"
                SLICES_DOC_, 2, ass_tokens->str);
        return e->nr;
    }

    ti_chain_move(&chain, &query->chain);

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    query->rval = NULL;

    if (index__read_slice_indices(query, slice, &start, &stop, &step, e))
        goto fail1;

    if (step != 1)
    {
        ex_set(e, EX_BAD_DATA,
                "slice assignments require a step value of 1 "
                "but got %zd instead"SLICES_DOC_,
                step);
        goto fail1;
    }

    if (ti_do_statement(query, ass_scope->children->next->node, e))
        goto fail1;

    if (!ti_val_is_array(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "slice assignments require an `"TI_VAL_ARR_S"` type "
                "but got type `%s` instead"SLICES_DOC_,
                ti_val_str(query->rval));
        goto fail1;
    }

    current_n = varr->vec->n;
    evarr = (ti_varr_t *) query->rval;

    c = stop > start ? stop - start : 0;
    n = evarr->vec->n;
    new_n = current_n + n - c;

    if (new_n > current_n && vec_resize(&varr->vec, new_n))
    {
        ex_set_mem(e);
        goto fail1;
    }

    memmove(
        varr->vec->data + start + n,
        varr->vec->data + start + c,
        (current_n - start - c) * sizeof(void*));
    memcpy(varr->vec->data + start, evarr->vec->data, n * sizeof(void*));

    if (evarr->ref == 1)
        evarr->vec->n = 0;
    else for (vec_each(evarr->vec, ti_val_t, v))
        ti_incref(v);

    ti_val_drop(query->rval);  /* evarr cannot be used anymore */
    query->rval = (ti_val_t *) ti_nil_get();

    varr->vec->n = new_n;

    if (ti_chain_is_set(&chain))
    {
        ti_task_t * task = ti_task_get_task(query->ev, chain.thing, e);
        if (!task)
            goto fail1;

        if (ti_task_add_splice(
                task,
                chain.name,
                varr,
                start,
                c,
                n))
            ex_set_mem(e);
    }

fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_drop((ti_val_t *) varr);
fail0:
    ti_chain_unset(&chain);
    return e->nr;

}

static int index__numeric(
        ti_query_t * query,
        cleri_node_t * scope,
        size_t * idx,
        size_t n,
        ex_t * e)
{
    ssize_t i;

    if (ti_do_statement(query, scope, e))
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

static int index__index_raw(ti_query_t * query, cleri_node_t * scope, ex_t * e)
{
    ti_raw_t * source = (ti_raw_t *) query->rval;
    size_t idx = 0;  /* only set to prevent warning */

    query->rval = NULL;

    if (index__numeric(query, scope, &idx, source->n, e))
        goto done;

    query->rval = (ti_val_t *) ti_raw_create(source->data + idx, 1);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int index__index_arr(ti_query_t * query, cleri_node_t * scope, ex_t * e)
{
    ti_varr_t * source = (ti_varr_t *) query->rval;
    size_t idx = 0;  /* only set to prevent warning */

    query->rval = NULL;

    if (index__numeric(query, scope, &idx, source->vec->n, e))
        goto done;

    query->rval = vec_get(source->vec, idx);
    ti_incref(query->rval);

done:
    ti_val_drop((ti_val_t *) source);
    return e->nr;
}

static int index__array_ass(ti_query_t * query, cleri_node_t * inode, ex_t * e)
{
    cleri_node_t * idx_scope = inode->children->next->node->children->node;
    cleri_node_t * ass_scope = inode->children->next->next->next->node;
    cleri_node_t * ass_tokens = ass_scope->children->node;
    ti_varr_t * varr;
    ti_chain_t chain;
    size_t idx = 0;  /* only set to prevent warning */

    ti_chain_move(&chain, &query->chain);

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (index__numeric(query, idx_scope, &idx, varr->vec->n, e) ||
        ti_do_statement(query, ass_scope->children->next->node, e))
        goto fail1;

    if (ass_tokens->len == 2)
    {
        ti_val_t * val = vec_get(varr->vec, idx);
        if (ti_opr_a_to_b(val, ass_tokens, &query->rval, e))
            goto fail1;

        ti_val_drop(val);
        varr->vec->data[idx] = query->rval;
    }
    else if(ti_varr_set(varr, (void **) &query->rval, idx, e))
        goto fail1;

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

static int index__get(ti_query_t * query, cleri_node_t * scope, ex_t * e)
{
    ti_prop_t * prop;
    ti_thing_t * thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, scope, e))
        goto fail0;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "expecting an index of type `"TI_VAL_RAW_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        goto fail0;
    }

    prop = ti_thing_weak_get_e(thing, (ti_raw_t *) query->rval, e);
    if (!prop)
        goto fail0;

    if (thing->id)
        ti_chain_set(&query->chain, thing, prop->name);

    ti_val_drop(query->rval);
    query->rval = prop->val;
    ti_incref(query->rval);

fail0:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int index__set(ti_query_t * query, cleri_node_t * inode, ex_t * e)
{
    cleri_node_t * idx_scope = inode->children->next->node->children->node;
    cleri_node_t * ass_scope = inode->children->next->next->next->node;
    cleri_node_t * ass_tokens = ass_scope->children->node;
    ti_prop_t * prop;
    ti_thing_t * thing;
    ti_name_t * name;
    ti_raw_t * rname;
    size_t max_props = query->target
            ? query->target->quota->max_props
            : TI_QUOTA_NOT_SET;     /* check for target since assign is
                                       possible when chained in all scopes */

    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (thing->props->n == max_props)
    {
        ex_set(e, EX_MAX_QUOTA,
            "maximum properties quota of %zu has been reached"
            TI_SEE_DOC("#quotas"), max_props);
        goto fail0;
    }

    if (ti_do_statement(query, idx_scope, e))
        goto fail0;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "expecting an index of type `"TI_VAL_RAW_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        goto fail0;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, ass_scope->children->next->node, e))
        goto fail1;

    if (ass_tokens->len == 2)
    {
        prop = ti_thing_weak_get_e(thing, rname, e);
        if (!prop || ti_opr_a_to_b(prop->val, ass_tokens, &query->rval, e))
            goto fail1;

        ti_val_drop(prop->val);
        prop->val = query->rval;
    }
    else
    {
        if (ti_val_make_assignable(&query->rval, e))
            goto fail1;

        name = ti_names_get((const char *) rname->data, rname->n);
        if (!name)
        {
            ex_set_mem(e);
            goto fail1;
        }

        prop = ti_thing_prop_set_e(thing, name, query->rval, e);
        if (!prop)
        {
            assert (e->nr);
            ti_name_drop(name);
            goto fail1;
        }
    }

    ti_incref(prop->val);

    if (thing->id)
    {
        ti_task_t * task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto fail1;

        if (ti_task_add_set(task, prop->name, prop->val))
        {
            ex_set_mem(e);
            goto fail1;
        }
        ti_chain_set(&query->chain, thing, prop->name);
    }

fail1:
    ti_val_drop((ti_val_t *) rname);
fail0:
    ti_val_unlock((ti_val_t *) thing, true /* lock_was_set */);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

int ti_index(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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
                ? index__slice_raw(query, slice, e)
                : index__index_raw(query, slice->children->node, e);

    case TI_VAL_ARR:
        if (do_assign && ti_varr_is_tuple((ti_varr_t *) val))
            goto assign_error;

        return do_assign
                ? do_slice
                    ? index__slice_ass(query, nd, e)
                    : index__array_ass(query, nd, e)
                : do_slice
                    ? index__slice_arr(query, slice, e)
                    : index__index_arr(query, slice->children->node, e);

    case TI_VAL_THING:
        if (do_slice)
            goto slice_error;

        return do_assign
                ? index__set(query, nd, e)
                : index__get(query, slice->children->node, e);

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
