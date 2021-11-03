/*
 * ti/index.c
 */
#include <langdef/langdef.h>
#include <ti/do.h>
#include <ti/field.h>
#include <ti/index.h>
#include <ti/method.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr/oprinc.h>
#include <ti/prop.h>
#include <ti/task.h>
#include <ti/thing.inline.h>
#include <ti/val.inline.h>
#include <ti/vint.h>

static inline size_t index__slice_also_not_nil(ti_val_t * val, ex_t * e)
{
    if (!ti_val_is_nil(val))
        ex_set(e, EX_TYPE_ERROR,
               "slice indices must be of type `"TI_VAL_INT_S"` "
               "or `"TI_VAL_NIL_S"` but got got type `%s` instead"
               DOC_SLICES, ti_val_str(val));
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
            ssize_t i = VINT(query->rval);
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

        ti_val_unsafe_drop(query->rval);
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
            ssize_t i = VINT(query->rval);
            if (i < n)
            {
                *stop = i < 0 ? ((n + i) < 0 ? 0 : n + i) : i;
            }
            stop_ = stop;
        }
        else if (index__slice_also_not_nil(query->rval, e))
            return e->nr;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        child = child->next;
        if (!child)
            return e->nr;
    }

    assert (child);  /* ':' */

    child = child->next;

    if (child)  /* must be a statement since no more indices are allowed */
    {
        assert (child->node->cl_obj->gid == CLERI_GID_STATEMENT);

        if (ti_do_statement(query, child->node, e))
            return e->nr;

        if (ti_val_is_int(query->rval))
        {
            ssize_t i = VINT(query->rval);
            if (i < 0)
            {
                if (!start_)
                    *start = n -1;
                if (!stop_)
                    *stop = -1;
            }
            else if (i == 0)
            {
                ex_set(e, EX_VALUE_ERROR, "slice step cannot be zero");
                return e->nr;
            }
            *step = i;
        }
        else if (index__slice_also_not_nil(query->rval, e))
            return e->nr;

        ti_val_unsafe_drop(query->rval);
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
    ti_val_unsafe_drop((ti_val_t *) source);
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
    ti_val_unsafe_drop((ti_val_t *) source);
    return e->nr;
}

static int index__slice_ass(ti_query_t * query, cleri_node_t * inode, ex_t * e)
{
    cleri_node_t * slice = inode->children->next->node;
    cleri_node_t * ass_statem = inode->children->next->next->next->node;
    cleri_node_t * ass_tokens = ass_statem->children->node;
    ti_varr_t * evarr, * varr = (ti_varr_t *) query->rval;
    ssize_t c, n;
    ssize_t start = 0, stop = (ssize_t) varr->vec->n, step = 1;
    uint32_t current_n, new_n;

    if (ass_tokens->len == 2)
    {
        ex_set(e, EX_TYPE_ERROR,
                "unsupported operand type `%.*s` for slice assignments"
                DOC_SLICES, 2, ass_tokens->str);
        return e->nr;
    }

    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    query->rval = NULL;

    if (index__read_slice_indices(query, slice, &start, &stop, &step, e))
        goto fail1;

    if (step != 1)
    {
        ex_set(e, EX_VALUE_ERROR,
                "slice assignments require a step value of 1 "
                "but got %zd instead"DOC_SLICES,
                step);
        goto fail1;
    }

    if (ti_do_statement(query, ass_statem->children->next->node, e))
        goto fail1;

    if (!ti_val_is_array(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "slice assignments require a `"TI_VAL_LIST_S"` or "
                "`"TI_VAL_TUPLE_S"` type but got type `%s` instead"DOC_SLICES,
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

    for (step = start; step < stop; ++step)
        ti_val_unsafe_gc_drop(VEC_get(varr->vec, step));

    memmove(
        varr->vec->data + start + n,
        varr->vec->data + start + c,
        (current_n - start - c) * sizeof(void*));
    memcpy(varr->vec->data + start, evarr->vec->data, n * sizeof(void*));

    if (evarr->ref == 1)
        evarr->vec->n = 0;
    else for (vec_each(evarr->vec, ti_val_t, v))
        ti_incref(v);

    ti_val_unsafe_drop(query->rval);  /* evarr cannot be used anymore */
    query->rval = (ti_val_t *) ti_nil_get();

    varr->vec->n = new_n;

    if (varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, varr->parent);
        if (!task || ti_task_add_splice(
                task,
                ti_varr_key(varr),
                varr,
                (uint32_t) start,
                (uint32_t) c,
                (uint32_t) n))
            ex_set_mem(e);
    }

fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;

}

static int index__numeric(
        ti_query_t * query,
        cleri_node_t * statement,
        size_t * idx,
        size_t n,
        ex_t * e)
{
    ssize_t i;

    if (ti_do_statement(query, statement, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting an index of type `"TI_VAL_INT_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    i = VINT(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (i < 0)
        i += n;

    if (i < 0 || i >= (ssize_t) n)
        ex_set(e, EX_LOOKUP_ERROR, "index out of range");
    else
        *idx = (size_t) i;

    return e->nr;
}

static int index__index_raw(
        ti_query_t * query,
        cleri_node_t * statement,
        ex_t * e)
{
    ti_raw_t * source = (ti_raw_t *) query->rval;
    size_t idx = 0;  /* only set to prevent warning */

    query->rval = NULL;

    if (index__numeric(query, statement, &idx, source->n, e))
        goto done;

    query->rval = (ti_val_t *) ti_raw_create(source->tp, source->data+idx, 1);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_unsafe_drop((ti_val_t *) source);
    return e->nr;
}

static int index__index_arr(
        ti_query_t * query,
        cleri_node_t * statement,
        ex_t * e)
{
    ti_varr_t * source = (ti_varr_t *) query->rval;
    size_t idx = 0;  /* only set to prevent warning */

    query->rval = NULL;

    if (index__numeric(query, statement, &idx, source->vec->n, e))
        goto done;

    query->rval = VEC_get(source->vec, idx);
    ti_incref(query->rval);

done:
    ti_val_unsafe_drop((ti_val_t *) source);
    return e->nr;
}

static int index__array_ass(ti_query_t * query, cleri_node_t * inode, ex_t * e)
{
    cleri_node_t * idx_statem = inode->children->next->node->children->node;
    cleri_node_t * ass_statem = inode->children->next->next->next->node;
    cleri_node_t * ass_tokens = ass_statem->children->node;
    ti_varr_t * varr;
    size_t idx = 0;  /* only set to prevent warning */

    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (index__numeric(query, idx_statem, &idx, varr->vec->n, e) ||
        ti_do_statement(query, ass_statem->children->next->node, e))
        goto fail1;

    if (ass_tokens->len == 2)
    {
        ti_val_t * val = VEC_get(varr->vec, idx);
        if (ti_opr_a_to_b(val, ass_tokens, &query->rval, e))
            goto fail1;

        ti_val_unsafe_drop(val);
        varr->vec->data[idx] = query->rval;
    }
    else if(ti_varr_set(varr, (void **) &query->rval, idx, e))
        goto fail1;

    ti_incref(query->rval);

    if (varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, varr->parent);
        if (!task || ti_task_add_splice(
                task,
                ti_varr_key(varr),
                varr,
                (uint32_t) idx,
                1,
                1))
            ex_set_mem(e);
    }

fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}

static int index__get(ti_query_t * query, cleri_node_t * statement, ex_t * e)
{
    ti_witem_t witem;
    ti_thing_t * thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, statement, e))
        goto fail0;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting an index of type `"TI_VAL_STR_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        goto fail0;
    }

    if (ti_thing_get_by_raw_e(&witem, thing, (ti_raw_t *) query->rval, e))
        goto fail0;

    ti_val_unsafe_drop(query->rval);
    query->rval = *witem.val;
    ti_incref(query->rval);

fail0:
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static inline int index__o_upd_prop(
        ti_witem_t * witem,
        ti_query_t * query,
        ti_thing_t * thing,
        ti_raw_t * rname,
        cleri_node_t * tokens_nd,
        ex_t * e)
{
    return (ti_thing_get_by_raw_e(witem, thing, rname, e) ||
            ti_opr_a_to_b(*witem->val, tokens_nd, &query->rval, e)
    ) ? e->nr : 0;
}

static inline int index__t_upd_prop(
        ti_wprop_t * wprop,
        ti_query_t * query,
        ti_thing_t * thing,
        ti_raw_t * rname,
        cleri_node_t * tokens_nd,
        ex_t * e)
{
    ti_field_t * field;
    ti_name_t * name = ti_names_weak_from_raw(rname);

    if (name && (field = ti_field_by_name(thing->via.type, name)))
    {
        wprop->name = field->name;
        wprop->val = (ti_val_t **) vec_get_addr(thing->items.vec, field->idx);

        return (
            ti_opr_a_to_b(*wprop->val, tokens_nd, &query->rval, e) ||
            ti_field_make_assignable(field, &query->rval, thing, e)
        ) ? e->nr : 0;
    }

    ti_thing_t_set_not_found(thing, name, rname, e);
    return e->nr;
}

static inline int index__upd_prop(
        ti_witem_t * witem,
        ti_query_t * query,
        ti_thing_t * thing,
        ti_raw_t * rname,
        cleri_node_t * tokens_nd,
        ex_t * e)
{
    if (ti_thing_is_object(thing)
            ? index__o_upd_prop(witem, query, thing, rname, tokens_nd, e)
            : index__t_upd_prop(
                    (ti_wprop_t *) witem,
                    query, thing, rname, tokens_nd, e))
        return e->nr;

    ti_val_unsafe_gc_drop(*witem->val);
    *witem->val = query->rval;
    ti_incref(query->rval);

    return 0;
}

static int index__set(ti_query_t * query, cleri_node_t * inode, ex_t * e)
{
    cleri_node_t * idx_statem = inode->children->next->node->children->node;
    cleri_node_t * ass_statem = inode->children->next->next->next->node;
    cleri_node_t * ass_tokens = ass_statem->children->node;
    ti_witem_t witem;
    ti_thing_t * thing;
    ti_raw_t * rname;

    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, idx_statem, e))
        goto fail0;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting an index of type `"TI_VAL_STR_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        goto fail0;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, ass_statem->children->next->node, e))
        goto fail1;

    if (ass_tokens->len == 2
            ? index__upd_prop(&witem, query, thing, rname, ass_tokens, e)
            : ti_thing_set_val_from_strn(
                    &witem,
                    thing,
                    (const char *) rname->data,
                    rname->n,
                    &query->rval,
                    e))
        goto fail1;

    if (thing->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_set(task, witem.key, *witem.val))
            ex_set_mem(e);
    }

fail1:
    ti_val_unsafe_drop((ti_val_t *) rname);
fail0:
    ti_val_unlock((ti_val_t *) thing, true /* lock_was_set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
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

    /* TODO: check if we can support slice on bytes type */
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NAME:
    case TI_VAL_STR:
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
    case TI_VAL_TEMPLATE:
        assert(0);
        /* fall through */
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_SET:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_MPDATA:
    case TI_VAL_CLOSURE:
    case TI_VAL_FUTURE:
        if (do_slice)
            goto slice_error;
        goto index_error;
    }

    assert (0);
    return e->nr;

slice_error:
    ex_set(e, EX_TYPE_ERROR, "type `%s` has no slice support",
            ti_val_str(val));
    return e->nr;

index_error:
    ex_set(e, EX_TYPE_ERROR, "type `%s` is not indexable",
            ti_val_str(val));
    return e->nr;

assign_error:
    ex_set(e, EX_TYPE_ERROR, "type `%s` does not support index assignments",
        ti_val_str(val));
    return e->nr;
}
