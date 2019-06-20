/*
 * ti/cq.c

 * Functions:
 *  1. Test if the function is available in scope
 *  2. Test the number of arguments
 *  3. Lazy evaluate each argument
 *
 */
#include <ti/cfn/fnassert.h>
#include <ti/cfn/fnblob.h>
#include <ti/cfn/fnbool.h>
#include <ti/cfn/fndel.h>
#include <ti/cfn/fnendswith.h>
#include <ti/cfn/fnfilter.h>
#include <ti/cfn/fnfind.h>
#include <ti/cfn/fnfindindex.h>
#include <ti/cfn/fnhas.h>
#include <ti/cfn/fnhasprop.h>
#include <ti/cfn/fnid.h>
#include <ti/cfn/fnindexof.h>
#include <ti/cfn/fnint.h>
#include <ti/cfn/fnisarray.h>
#include <ti/cfn/fnisascii.h>
#include <ti/cfn/fnisbool.h>
#include <ti/cfn/fnisfloat.h>
#include <ti/cfn/fnisinf.h>
#include <ti/cfn/fnisint.h>
#include <ti/cfn/fnislist.h>
#include <ti/cfn/fnisnan.h>
#include <ti/cfn/fnisraw.h>
#include <ti/cfn/fnistuple.h>
#include <ti/cfn/fnisutf8.h>
#include <ti/cfn/fnlen.h>
#include <ti/cfn/fnlower.h>
#include <ti/cfn/fnmap.h>
#include <ti/cfn/fnnow.h>
#include <ti/cfn/fnpop.h>
#include <ti/cfn/fnpush.h>
#include <ti/cfn/fnrefs.h>
#include <ti/cfn/fnremove.h>
#include <ti/cfn/fnrename.h>
#include <ti/cfn/fnret.h>
#include <ti/cfn/fnset.h>
#include <ti/cfn/fnsplice.h>
#include <ti/cfn/fnstartswith.h>
#include <ti/cfn/fnstr.h>
#include <ti/cfn/fnt.h>
#include <ti/cfn/fntest.h>
#include <ti/cfn/fntry.h>
#include <ti/cfn/fnupper.h>

static int cq__function(
        ti_query_t * query,
        cleri_node_t * nd,
        _Bool is_scope,             /* scope or chain */
        ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or name node */

    params = nd                             /* sequence */
            ->children->next->next->node;   /* list of scope (arguments) */


    switch (fname->cl_obj->gid)
    {
    case CLERI_GID_F_ASSERT:
        if (is_scope)
            return cq__f_assert(query, params, e);
        break;
    case CLERI_GID_F_BLOB:
        if (is_scope)
            return cq__f_blob(query, params, e);
        break;
    case CLERI_GID_F_BOOL:
        if (is_scope)
            return cq__f_bool(query, params, e);
        break;
    case CLERI_GID_F_ENDSWITH:
        return cq__f_endswith(query, params, e);
    case CLERI_GID_F_DEL:
        return cq__f_del(query, params, e);
    case CLERI_GID_F_FILTER:
        return cq__f_filter(query, params, e);
    case CLERI_GID_F_FIND:
        return cq__f_find(query, params, e);
    case CLERI_GID_F_FINDINDEX:
        return cq__f_findindex(query, params, e);
    case CLERI_GID_F_HAS:
        return cq__f_has(query, params, e);
    case CLERI_GID_F_HASPROP:
        return cq__f_hasprop(query, params, e);
    case CLERI_GID_F_ID:
        return cq__f_id(query, params, e);
    case CLERI_GID_F_INDEXOF:
        return cq__f_indexof(query, params, e);
    case CLERI_GID_F_INT:
        if (is_scope)
            return cq__f_int(query, params, e);
        break;
    case CLERI_GID_F_ISARRAY:
        if (is_scope)
            return cq__f_isarray(query, params, e);
        break;
    case CLERI_GID_F_ISASCII:
        if (is_scope)
            return cq__f_isascii(query, params, e);
        break;
    case CLERI_GID_F_ISBOOL:
        if (is_scope)
            return cq__f_isbool(query, params, e);
        break;
    case CLERI_GID_F_ISFLOAT:
        if (is_scope)
            return cq__f_isfloat(query, params, e);
        break;
    case CLERI_GID_F_ISINF:
        if (is_scope)
            return cq__f_isinf(query, params, e);
        break;
    case CLERI_GID_F_ISINT:
        if (is_scope)
            return cq__f_isint(query, params, e);
        break;
    case CLERI_GID_F_ISLIST:
        if (is_scope)
            return cq__f_islist(query, params, e);
        break;
    case CLERI_GID_F_ISNAN:
        if (is_scope)
            return cq__f_isnan(query, params, e);
        break;
    case CLERI_GID_F_ISRAW:
        if (is_scope)
            return cq__f_israw(query, params, e);
        break;
    case CLERI_GID_F_ISTUPLE:
        if (is_scope)
            return cq__f_istuple(query, params, e);
        break;
    case CLERI_GID_F_ISSTR:
    case CLERI_GID_F_ISUTF8:
        if (is_scope)
            return cq__f_isutf8(query, params, e);
        break;
    case CLERI_GID_F_LEN:
        return cq__f_len(query, params, e);
    case CLERI_GID_F_LOWER:
        return cq__f_lower(query, params, e);
    case CLERI_GID_F_MAP:
        return cq__f_map(query, params, e);
    case CLERI_GID_F_NOW:
        if (is_scope)
            return cq__f_now(query, params, e);
        break;
    case CLERI_GID_F_POP:
        return cq__f_pop(query, params, e);
    case CLERI_GID_F_PUSH:
        return cq__f_push(query, params, e);
    case CLERI_GID_F_REFS:
        if (is_scope)
            return cq__f_refs(query, params, e);
        break;
    case CLERI_GID_F_REMOVE:
        return cq__f_remove(query, params, e);
    case CLERI_GID_F_RENAME:
        return cq__f_rename(query, params, e);
    case CLERI_GID_F_RET:
        return cq__f_ret(query, params, e);
    case CLERI_GID_F_SET:
        if (is_scope)
            return cq__f_set(query, params, e);
        break;
    case CLERI_GID_F_SPLICE:
        return cq__f_splice(query, params, e);
    case CLERI_GID_F_STARTSWITH:
        return cq__f_startswith(query, params, e);
    case CLERI_GID_F_STR:
        if (is_scope)
            return cq__f_str(query, params, e);
        break;
    case CLERI_GID_F_T:
        if (is_scope)
            return cq__f_t(query, params, e);
        break;
    case CLERI_GID_F_TEST:
        return cq__f_test(query, params, e);
    case CLERI_GID_F_TRY:
        if (is_scope)
            return cq__f_try(query, params, e);
        break;
    case CLERI_GID_F_UPPER:
        return cq__f_upper(query, params, e);
    }

    /* set error */
    if (is_scope)
    {
        ex_set(e, EX_INDEX_ERROR,
                "unknown function `%.*s`",
                fname->len,
                fname->str);
    }
    else
    {
        ti_val_t * val = ti_query_val_pop(query);
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `%.*s`",
                ti_val_str(val),
                fname->len,
                fname->str);
        ti_val_drop(val);
    }

    return e->nr;
}

static int cq__array(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARRAY);

    ti_varr_t * varr;
    uintptr_t sz = (uintptr_t) nd->data;
    cleri_children_t * child = nd          /* sequence */
            ->children->next->node         /* list */
            ->children;

    if (sz >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached"
                TI_SEE_DOC("#quotas"), query->target->quota->max_array_size);
        return e->nr;
    }

    varr = ti_varr_create(sz);
    if (!varr)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    for (; child; child = child->next->next)
    {
        if (ti_cq_scope(query, child->node, e))
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

static int cq__assignment(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);
    assert (query->ev);

    ti_thing_t * thing;
    ti_name_t * name;
    ti_task_t * task = NULL;
    ti_val_t * left_val = NULL;     /* assign to prevent warning */
    size_t max_props = query->target->quota->max_props;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    if (assign_nd->len == 1)
    {
        /* assign_nd == '=', a new assignment */
        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum properties quota of %zu has been reached"
                    TI_SEE_DOC("#quotas"), max_props);
            goto fail1;
        }
    }
    else
    {
        /* apply to existing variable */
        assert (assign_nd->len == 2);
        assert (assign_nd->str[1] == '=');

        left_val = ti_thing_prop_weak_get(thing, name);
        if (!left_val)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "`%.*s` is undefined", (int) name->n, name->str);
            goto fail1;
        }
    }

    /* should also work in chain because then scope->local must be NULL */
    if (    ti_scope_has_local_name(query->scope, name) ||
            ti_scope_in_use_name(query->scope, thing, name))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot assign a new value to `%.*s` while the property is in use",
            (int) name->n, name->str);
        goto fail1;
    }

    if (ti_cq_scope(query, scope_nd, e))
        goto fail1;

    if (assign_nd->len == 2)
    {
        assert (left_val);
        if (ti_opr_a_to_b(left_val, assign_nd, &query->rval, e))
            goto fail1;
    }
    else if (ti_val_make_assignable(&query->rval, e))
        goto fail1;

    if (thing->id)
    {
        task = ti_task_get_task(query->ev, thing, e);

        if (!task)
            goto fail1;

        if (ti_task_add_assign(task, name, query->rval))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    if (ti_thing_prop_set(thing, name, query->rval))
    {
        if (task)
            free(vec_pop(task->jobs));
        goto alloc_err;
    }

    ti_incref(query->rval);

    goto done;

alloc_err:
    ex_set_alloc(e);

fail1:
    ti_name_drop(name);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int cq__index(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_INDEX);
    assert (nd->children);

    cleri_children_t * child;
    cleri_node_t * node;
    ti_val_t * val = ti_query_val_pop(query);
    int64_t idx;
    ssize_t n;

    /* multiple indexes are possible, for example x[0][0] */
    for (child = nd->children; child; child = child->next)
    {
        node = child->node              /* sequence  [ int ]  */
            ->children->next->node;     /* scope */

        if (ti_cq_scope(query, node, e))
            return e->nr;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                    "expecting an index to be of "
                    "type `"TI_VAL_INT_S"` but got type `%s` instead",
                    ti_val_str(query->rval));
            return e->nr;
        }

        idx = ((ti_vint_t * ) query->rval)->int_;

        switch (val->tp)
        {
        case TI_VAL_RAW:
            n = ((ti_raw_t *) val)->n;
            break;
        case TI_VAL_ARR:
            n = ((ti_varr_t *) val)->vec->n;
            break;
        default:
            ex_set(e, EX_BAD_DATA, "type `%s` is not indexable",
                    ti_val_str(val));
            goto done;
        }

        if (idx < 0)
            idx += n;

        if (idx < 0 || idx >= n)
        {
            ex_set(e, EX_INDEX_ERROR, "index out of range");
            goto done;
        }

        ti_val_drop(query->rval);

        switch (val->tp)
        {
        case TI_VAL_RAW:
            query->rval = (ti_val_t *) ti_vint_create(
                    (int64_t) ((ti_raw_t *) val)->data[idx]);
            if (!query->rval)
            {
                ex_set_alloc(e);
                goto done;
            }
            break;
        case TI_VAL_ARR:
            query->rval = vec_get(((ti_varr_t *) val)->vec, idx);
            ti_incref(query->rval);
            break;
        default:
            assert (0);
            ex_set_internal(e);
            goto done;
        }

        if (!child->next)
            break;

        ti_val_drop(val);
        val = query->rval;
        query->rval = NULL;
    }

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__chain_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    ti_name_t * name;
    ti_val_t * val;
    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no property `%.*s`",
                ti_val_str((ti_val_t *) thing),
                (int) nd->len, nd->str);
        goto done;
    }

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_thing_prop_weak_get(thing, name) : NULL;

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "thing "TI_THING_ID" has no property `%.*s`",
                thing->id, (int) nd->len, nd->str);
        goto done;
    }

    ti_scope_set_name_val(query->scope, name, val);

done:
    ti_val_drop((ti_val_t *) thing);

    return e->nr;
}

static int cq__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);
    assert (query->rval);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* function, assignment,
                                               name */

    if (ti_val_is_thing(query->rval))
    {
        ti_scope_t * tmp_scope = ti_scope_enter(
                query->scope,
                (ti_thing_t *) query->rval);

        if (!tmp_scope)
        {
            ex_set_alloc(e);
            return e->nr;
        }

        query->scope = tmp_scope;
    }

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        if (cq__function(query, node, false, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (cq__assignment(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (cq__chain_name(query, node, e))
            return e->nr;
        break;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    node = child->node;

    if (node->children && cq__index(query, node, e))
        return e->nr;

    child = child->next;
    if (!child)
        return e->nr;

    (void) cq__chain(query, child->node, e);
    return e->nr;
}

static int cq__closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CLOSURE);

    query->rval = (ti_val_t *) ti_closure_from_node(nd);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int cq__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    uint32_t gid;
    ti_val_t * a_val = NULL;
    assert( nd->cl_obj->tp == CLERI_TP_RULE ||
            nd->cl_obj->tp == CLERI_TP_PRIO ||
            nd->cl_obj->tp == CLERI_TP_THIS);

    nd = nd->cl_obj->tp == CLERI_TP_PRIO ?
            nd                          /* prio */
            ->children->node :          /* compare sequence */
            nd                          /* rule/this */
            ->children->node            /* prio */
            ->children->node;           /* compare sequence */

    assert (nd->cl_obj->tp == CLERI_TP_SEQUENCE);
    assert (query->rval == NULL);

    if (nd->cl_obj->gid == CLERI_GID_SCOPE)
        return ti_cq_scope(query, nd, e);

    gid = nd->children->next->node->children->node->cl_obj->gid;

    switch (gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_AND:
    case CLERI_GID_OPR3_BITWISE_XOR:
    case CLERI_GID_OPR4_BITWISE_OR:
    case CLERI_GID_OPR5_COMPARE:
        if (cq__operations(query, nd->children->node, e))
            return e->nr;
        a_val = query->rval;
        query->rval = NULL;
        if (cq__operations(query, nd->children->next->next->node, e))
            break;
        (void) ti_opr_a_to_b(a_val, nd->children->next->node, &query->rval, e);
        break;

    case CLERI_GID_OPR6_CMP_AND:
        if (    cq__operations(query, nd->children->node, e) ||
                !ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return cq__operations(query, nd->children->next->next->node, e);

    case CLERI_GID_OPR7_CMP_OR:
        if (    cq__operations(query, nd->children->node, e) ||
                ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return cq__operations(query, nd->children->next->next->node, e);

    default:
        assert (0);
    }

    ti_val_drop(a_val);
    return e->nr;
}

static int cq__primitives(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_PRIMITIVES);
    assert (!e->nr);

    cleri_node_t * node = nd            /* choice */
            ->children->node;           /* false, nil, true, undefined,
                                           int, float, string */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_FALSE:
        query->rval = (ti_val_t *) ti_vbool_get(false);
        break;
    case CLERI_GID_T_FLOAT:
        if (!node->data)
        {
            node->data = ti_vfloat_create(strx_to_double(node->str));
            if (!node->data)
            {
                ex_set_alloc(e);
                return e->nr;
            }
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
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
                ex_set_alloc(e);
                return e->nr;
            }
            if (errno == ERANGE)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
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
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
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
                ex_set_alloc(e);
                return e->nr;
            }
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_TRUE:
        query->rval = (ti_val_t *) ti_vbool_get(true);
        break;
    }
    return e->nr;
}

/* changes scope->name and/or scope->thing */
static int cq__scope_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));
    assert (query->rval == NULL);

    ti_name_t * name;
    ti_val_t * val;

    name = ti_names_weak_get(nd->str, nd->len);

    /* first try to find the name in a local scopes */
    val = name ? ti_scope_find_local_val(query->scope, name) : NULL;

    /* next, try the root in case the `name` is not found */
    if (!val && name)
        val = ti_thing_prop_weak_get(query->target->root, name);

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "`%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    ti_scope_set_name_val(query->scope, name, val);

    return e->nr;
}

static int cq__scope_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence('{', List(Sequence(name, ':', scope)), '}')
     */
    assert (e->nr == 0);

    ti_thing_t * thing;
    cleri_children_t * child;
    size_t max_props = query->target->quota->max_props;
    size_t max_things = query->target->quota->max_things;

    if (query->target->things->n >= max_things)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum things quota of %zu has been reached"
                TI_SEE_DOC("#quotas"), max_things);
        return e->nr;
    }

    thing = ti_thing_create(0, query->target->things);
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

        if (ti_cq_scope(query, scope, e))
            goto err;

        name = ti_names_get(name_nd->str, name_nd->len);
        if (!name)
            goto alloc_err;

        if (ti_thing_prop_set(thing, name, query->rval))
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
    ex_set_alloc(e);
err:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

/* changes scope->name and/or scope->thing */
static int cq__tmp(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_TMP);
    assert (nd->len >= 2);
    assert (ti_name_is_valid_strn(nd->str + 1, nd->len - 1));
    assert (query->rval == NULL);

    ti_name_t * name;
    ti_prop_t * prop;

    name = ti_names_weak_get(nd->str, nd->len);
    prop = name ? ti_query_tmpprop_get(query, name) : NULL;

    if (!prop)
    {
        ex_set(e, EX_INDEX_ERROR,
                "temporary variable `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    ti_scope_set_name_val(query->scope, name, prop->val);

    return e->nr;
}

static int cq__tmp_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_TMP_ASSIGN);

    ti_thing_t * thing;
    ti_name_t * name;
    ti_prop_t * prop = NULL;     /* assign to prevent warning */
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    thing = (ti_thing_t *) ti_query_val_pop(query);
    assert (thing->tp == TI_VAL_THING);

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    if (assign_nd->len == 1)
        prop = NULL;
    else
    {
        /* apply to existing variable */
        assert (assign_nd->len == 2);
        assert (assign_nd->str[1] == '=');
        prop = ti_query_tmpprop_get(query, name);
        if (!prop)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "`%.*s` is undefined", (int) name->n, name->str);
            goto failed;
        }
        ti_decref(name);  /* we already have a reference to name */
    }

    /* should also work in chain because then scope->local must be NULL */
    if (    ti_scope_has_local_name(query->scope, name) ||
            ti_scope_in_use_name(query->scope, thing, name))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot assign a new value to `%.*s` while the variable is in use",
            (int) name->n, name->str);
        goto failed;
    }

    if (ti_cq_scope(query, scope_nd, e))
        goto failed;

    assert (query->rval);

    if (assign_nd->len == 2)
    {
        assert (prop);
        if (ti_opr_a_to_b(prop->val, assign_nd, &query->rval, e))
            goto failed;
    }

    if (prop)
    {
        ti_val_drop(prop->val);
        prop->val = query->rval;
    }
    else
    {
        prop = ti_prop_create(name, query->rval);
        if (!prop)
            goto alloc_err;

        if (query->tmpvars)
        {
            if (vec_push(&query->tmpvars, prop))
                goto alloc_err_with_prop;
        }
        else
        {
            query->tmpvars = vec_new(1);
            if (!query->tmpvars)
                goto alloc_err_with_prop;
            VEC_push(query->tmpvars, prop);
        }
    }

    ti_incref(query->rval);

    goto done;

alloc_err_with_prop:
    /* prop->name will be dropped and prop->val is still on query->rval */
    free(prop);

alloc_err:
    ex_set_alloc(e);

failed:
    ti_name_drop(name);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

int ti_cq_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);
    assert (query->rval == NULL);

    int nots = 0;
    cleri_node_t * node;
    ti_scope_t * current_scope;
    cleri_children_t * nchild, * child = nd         /* sequence */
            ->children;                             /* first child, not */

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, closure */
    current_scope = query->scope;
    query->scope = ti_scope_enter(current_scope, query->target->root);
    if (!query->scope)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        if (cq__array(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_CLOSURE:
        if (cq__closure(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (cq__assignment(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_FUNCTION:
        if (cq__function(query, node, true, e))
            goto onerror;
        break;
    case CLERI_GID_NAME:
        if (cq__scope_name(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (cq__operations(query, node->children->next->node, e))
            goto onerror;

        if (node->children->next->next->next)   /* optional (sequence) */
        {
            _Bool bool_ = ti_val_as_bool(query->rval);
            ti_val_drop(query->rval);
            query->rval = NULL;
            node = node->children->next->next->next->node;  /* sequence */
            if (ti_cq_scope(
                    query,
                    bool_
                        ? node->children->next->node        /* scope, true */
                        : node->children->next->next->next->node, /* false */
                    e))
                goto onerror;
        }
        break;
    case CLERI_GID_PRIMITIVES:
        if (cq__primitives(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_THING:
        if (cq__scope_thing(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_TMP:
        if (cq__tmp(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_TMP_ASSIGN:
        if (cq__tmp_assign(query, node, e))
            goto onerror;
        break;

    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    node = child->node;

    /* handle index */
    if (node->children && cq__index(query, node, e))
        goto onerror;

    /* chain */
    if (child->next && cq__chain(query, child->next->node, e))
        goto onerror;

    if (!query->rval)
    {
        /* The scope value must exist since only `TMP` and `NAME` are not
         * setting `query->rval` and they both set the scope value.
         */
        assert (query->scope->val);

        query->rval = query->scope->val;
        ti_incref(query->rval);
    }

    if (nots)
    {
        _Bool b = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_vbool_get((nots & 1) ^ b);
    }

onerror:
    ti_scope_leave(&query->scope, current_scope);
    return e->nr;
}

int ti_cq_optscope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->rval == NULL);
    if (!nd)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }
    return ti_cq_scope(query, nd, e);
}
