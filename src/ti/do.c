/*
 * ti/do.c
 */
#include <ti/auth.h>
#include <ti/do.h>
#include <ti/fn/fnadd.h>
#include <ti/fn/fnarray.h>
#include <ti/fn/fnassert.h>
#include <ti/fn/fnblob.h>
#include <ti/fn/fnbool.h>
#include <ti/fn/fncall.h>
#include <ti/fn/fncalle.h>
#include <ti/fn/fncollectioninfo.h>
#include <ti/fn/fncollectionsinfo.h>
#include <ti/fn/fncontains.h>
#include <ti/fn/fncounters.h>
#include <ti/fn/fndel.h>
#include <ti/fn/fndelcollection.h>
#include <ti/fn/fndelexpired.h>
#include <ti/fn/fndelprocedure.h>
#include <ti/fn/fndeltoken.h>
#include <ti/fn/fndeluser.h>
#include <ti/fn/fnendswith.h>
#include <ti/fn/fnerr.h>
#include <ti/fn/fnerrors.h>
#include <ti/fn/fnfilter.h>
#include <ti/fn/fnfind.h>
#include <ti/fn/fnfindindex.h>
#include <ti/fn/fnfloat.h>
#include <ti/fn/fngrant.h>
#include <ti/fn/fnhas.h>
#include <ti/fn/fnhasprop.h>
#include <ti/fn/fnid.h>
#include <ti/fn/fnindexof.h>
#include <ti/fn/fnint.h>
#include <ti/fn/fnisarray.h>
#include <ti/fn/fnisascii.h>
#include <ti/fn/fnisbool.h>
#include <ti/fn/fniserror.h>
#include <ti/fn/fnisfloat.h>
#include <ti/fn/fnisinf.h>
#include <ti/fn/fnisint.h>
#include <ti/fn/fnislist.h>
#include <ti/fn/fnisnan.h>
#include <ti/fn/fnisnil.h>
#include <ti/fn/fnisraw.h>
#include <ti/fn/fnisset.h>
#include <ti/fn/fnisthing.h>
#include <ti/fn/fnistuple.h>
#include <ti/fn/fnisutf8.h>
#include <ti/fn/fnlen.h>
#include <ti/fn/fnlower.h>
#include <ti/fn/fnmap.h>
#include <ti/fn/fnnewcollection.h>
#include <ti/fn/fnnewnode.h>
#include <ti/fn/fnnewprocedure.h>
#include <ti/fn/fnnewtoken.h>
#include <ti/fn/fnnewuser.h>
#include <ti/fn/fnnodeinfo.h>
#include <ti/fn/fnnodesinfo.h>
#include <ti/fn/fnnow.h>
#include <ti/fn/fnpop.h>
#include <ti/fn/fnpopnode.h>
#include <ti/fn/fnproceduredef.h>
#include <ti/fn/fnproceduredoc.h>
#include <ti/fn/fnprocedureinfo.h>
#include <ti/fn/fnproceduresinfo.h>
#include <ti/fn/fnpush.h>
#include <ti/fn/fnraise.h>
#include <ti/fn/fnrefs.h>
#include <ti/fn/fnremove.h>
#include <ti/fn/fnrename.h>
#include <ti/fn/fnrenamecollection.h>
#include <ti/fn/fnrenameuser.h>
#include <ti/fn/fnreplacenode.h>
#include <ti/fn/fnresetcounters.h>
#include <ti/fn/fnrevoke.h>
#include <ti/fn/fnset.h>
#include <ti/fn/fnsetloglevel.h>
#include <ti/fn/fnsetpassword.h>
#include <ti/fn/fnsetquota.h>
#include <ti/fn/fnshutdown.h>
#include <ti/fn/fnsplice.h>
#include <ti/fn/fnstartswith.h>
#include <ti/fn/fnstr.h>
#include <ti/fn/fnt.h>
#include <ti/fn/fntest.h>
#include <ti/fn/fntry.h>
#include <ti/fn/fntype.h>
#include <ti/fn/fnupper.h>
#include <ti/fn/fnuserinfo.h>
#include <ti/fn/fnusersinfo.h>
#include <ti/fn/fnwse.h>

/* maximum value we allow for the `deep` argument */
#define DO__MAX_DEEP_HINT 0x7f

#define do__no_chain_fn(__fn)                           \
if (is_chained)                                         \
    break;                                              \
return __fn(query, params, e)

#define do__collection_fn(__fn)                         \
if (is_chained)                                         \
    break;                                              \
if (do__no_collection_scope(query))                     \
    goto no_collection_scope;                           \
return __fn(query, params, e)

#define do__thingsdb_fn(__fn)                           \
if (is_chained)                                         \
    break;                                              \
if (do__no_thingsdb_scope(query))                       \
    goto no_thingsdb_scope;                             \
return __fn(query, params, e)

#define do__node_fn(__fn)                               \
if (is_chained)                                         \
    break;                                              \
if (do__no_node_scope(query))                           \
    goto no_node_scope;                                 \
return __fn(query, params, e)

#define do__thingsdb_or_collection_fn(__fn)             \
if (is_chained)                                         \
    break;                                              \
if (do__no_thingsdb_or_collection_scope(query))         \
    goto no_thingsdb_or_collection_scope;               \
return __fn(query, params, e)

static inline int do__no_node_scope(ti_query_t * query)
{
    return ~query->syntax.flags & TI_SYNTAX_FLAG_NODE;
}

static inline int do__no_thingsdb_scope(ti_query_t * query)
{
    return ~query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB;
}

static inline int do__no_collection_scope(ti_query_t * query)
{
    return ~query->syntax.flags & TI_SYNTAX_FLAG_COLLECTION;
}

static inline int do__no_thingsdb_or_collection_scope(ti_query_t * query)
{
    return query->syntax.flags & TI_SYNTAX_FLAG_NODE;
}

static int do__function(
        ti_query_t * query,
        cleri_node_t * nd,
        _Bool is_chained,             /* scope or chain */
        ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node;       /* name node */
    params = nd                             /* sequence */
            ->children->next->next->node;   /* list of scope (arguments) */

    switch ((ti_fn_enum_t) ((uintptr_t) fname->data))
    {
    case TI_FN_0:
        break;

   /* collection scope */
    case TI_FN_ADD:
        return do__f_add(query, params, e);
    case TI_FN_ARRAY:
        do__no_chain_fn(do__f_array);
    case TI_FN_ASSERT:
        do__no_chain_fn(do__f_assert);
    case TI_FN_BLOB:
        do__collection_fn(do__f_blob);
    case TI_FN_BOOL:
        do__no_chain_fn(do__f_bool);
    case TI_FN_CONTAINS:
        return do__f_contains(query, params, e);
    case TI_FN_ENDSWITH:
        return do__f_endswith(query, params, e);
    case TI_FN_ERR:
        do__no_chain_fn(do__f_err);
    case TI_FN_DEL:
        return do__f_del(query, params, e);
    case TI_FN_FILTER:
        return do__f_filter(query, params, e);
    case TI_FN_FIND:
        return do__f_find(query, params, e);
    case TI_FN_FINDINDEX:
        return do__f_findindex(query, params, e);
    case TI_FN_FLOAT:
        do__no_chain_fn(do__f_float);
    case TI_FN_HAS:
        return do__f_has(query, params, e);
    case TI_FN_HASPROP:
        return do__f_hasprop(query, params, e);
    case TI_FN_ID:
        return do__f_id(query, params, e);
    case TI_FN_INDEXOF:
        return do__f_indexof(query, params, e);
    case TI_FN_INT:
        do__no_chain_fn(do__f_int);
    case TI_FN_ISARRAY:
        do__no_chain_fn(do__f_isarray);
    case TI_FN_ISASCII:
        do__no_chain_fn(do__f_isascii);
    case TI_FN_ISBOOL:
        do__no_chain_fn(do__f_isbool);
    case TI_FN_ISERROR:
        do__no_chain_fn(do__f_iserror);
    case TI_FN_ISFLOAT:
        do__no_chain_fn(do__f_isfloat);
    case TI_FN_ISINF:
        do__no_chain_fn(do__f_isinf);
    case TI_FN_ISINT:
        do__no_chain_fn(do__f_isint);
    case TI_FN_ISLIST:
        do__no_chain_fn(do__f_islist);
    case TI_FN_ISNAN:
        do__no_chain_fn(do__f_isnan);
    case TI_FN_ISNIL:
        do__no_chain_fn(do__f_isnil);
    case TI_FN_ISRAW:
        do__no_chain_fn(do__f_israw);
    case TI_FN_ISSET:
        do__no_chain_fn(do__f_isset);
    case TI_FN_ISTHING:
        do__no_chain_fn(do__f_isthing);
    case TI_FN_ISTUPLE:
        do__no_chain_fn(do__f_istuple);
    case TI_FN_ISSTR:
    case TI_FN_ISUTF8:
        do__no_chain_fn(do__f_isutf8);
    case TI_FN_LEN:
        return do__f_len(query, params, e);
    case TI_FN_LOWER:
        return do__f_lower(query, params, e);
    case TI_FN_MAP:
        return do__f_map(query, params, e);
    case TI_FN_NOW:
        do__no_chain_fn(q__f_now);
    case TI_FN_POP:
        return do__f_pop(query, params, e);
    case TI_FN_PUSH:
        return do__f_push(query, params, e);
    case TI_FN_RAISE:
        do__no_chain_fn(do__f_raise);
    case TI_FN_REFS:
        do__no_chain_fn(do__f_refs);
    case TI_FN_REMOVE:
        return do__f_remove(query, params, e);
    case TI_FN_RENAME:
        return do__f_rename(query, params, e);
    case TI_FN_SET:
        do__no_chain_fn(do__f_set);
    case TI_FN_SPLICE:
        return do__f_splice(query, params, e);
    case TI_FN_STARTSWITH:
        return do__f_startswith(query, params, e);
    case TI_FN_STR:
        do__no_chain_fn(do__f_str);
    case TI_FN_T:
        do__collection_fn(do__f_t);
    case TI_FN_TEST:
        return do__f_test(query, params, e);
    case TI_FN_TRY:
        do__no_chain_fn(do__f_try);
    case TI_FN_TYPE:
        do__no_chain_fn(do__f_type);
    case TI_FN_UPPER:
        return do__f_upper(query, params, e);
    case TI_FN_WSE:
        do__no_chain_fn(do__f_wse);

    /* both thingsdb and collection scope */
    case TI_FN_CALL:
        do__thingsdb_or_collection_fn(do__f_call);
    case TI_FN_CALLE:
        do__thingsdb_or_collection_fn(do__f_calle);
    case TI_FN_DEL_PROCEDURE:
        do__thingsdb_or_collection_fn(do__f_del_procedure);
    case TI_FN_NEW_PROCEDURE:
        do__thingsdb_or_collection_fn(do__f_new_procedure);
    case TI_FN_PROCEDURES_INFO:
        do__thingsdb_or_collection_fn(do__f_procedures_info);
    case TI_FN_PROCEDURE_DEF:
        do__thingsdb_or_collection_fn(do__f_procedure_def);
    case TI_FN_PROCEDURE_DOC:
        do__thingsdb_or_collection_fn(do__f_procedure_doc);
    case TI_FN_PROCEDURE_INFO:
        do__thingsdb_or_collection_fn(do__f_procedure_info);

    /* thingsdb scope */
    case TI_FN_COLLECTION_INFO:
        do__thingsdb_fn(do__f_collection_info);
    case TI_FN_COLLECTIONS_INFO:
        do__thingsdb_fn(do__f_collections_info);
    case TI_FN_DEL_COLLECTION:
        do__thingsdb_fn(do__f_del_collection);
    case TI_FN_DEL_EXPIRED:
        do__thingsdb_fn(do__f_del_expired);
    case TI_FN_DEL_TOKEN:
        do__thingsdb_fn(do__f_del_token);
    case TI_FN_DEL_USER:
        do__thingsdb_fn(do__f_del_user);
    case TI_FN_GRANT:
        do__thingsdb_fn(do__f_grant);
    case TI_FN_NEW_COLLECTION:
        do__thingsdb_fn(do__f_new_collection);
    case TI_FN_NEW_NODE:
        do__thingsdb_fn(do__f_new_node);
    case TI_FN_NEW_TOKEN:
        do__thingsdb_fn(do__f_new_token);
    case TI_FN_NEW_USER:
        do__thingsdb_fn(do__f_new_user);
    case TI_FN_POP_NODE:
        do__thingsdb_fn(do__f_pop_node);
    case TI_FN_RENAME_COLLECTION:
        do__thingsdb_fn(do__f_rename_collection);
    case TI_FN_RENAME_USER:
        do__thingsdb_fn(do__f_rename_user);
    case TI_FN_REPLACE_NODE:
        do__thingsdb_fn(do__f_replace_node);
    case TI_FN_REVOKE:
        do__thingsdb_fn(do__f_revoke);
    case TI_FN_SET_PASSWORD:
        do__thingsdb_fn(do__f_set_password);
    case TI_FN_SET_QUOTA:
        do__thingsdb_fn(do__f_set_quota);
    case TI_FN_USER_INFO:
        do__thingsdb_fn(do__f_user_info);
    case TI_FN_USERS_INFO:
        do__thingsdb_fn(do__f_users_info);

    /* node scope */
    case TI_FN_COUNTERS:
        do__node_fn(do__f_counters);
    case TI_FN_NODE_INFO:
        do__node_fn(do__f_node_info);
    case TI_FN_NODES_INFO:
        do__node_fn(do__f_nodes_info);
    case TI_FN_RESET_COUNTERS:
        do__node_fn(do__f_reset_counters);
    case TI_FN_SET_LOG_LEVEL:
        do__node_fn(do__f_set_log_level);
    case TI_FN_SHUTDOWN:
        do__node_fn(do__f_shutdown);

    /* explicit error functions */
    case TI_FN_OVERFLOW_ERR:
        do__no_chain_fn(do__f_overflow_err);
    case TI_FN_ZERO_DIV_ERR:
        do__no_chain_fn(do__f_zero_div_err);
    case TI_FN_MAX_QUOTA_ERR:
        do__no_chain_fn(do__f_max_quota_err);
    case TI_FN_AUTH_ERR:
        do__no_chain_fn(do__f_auth_err);
    case TI_FN_FORBIDDEN_ERR:
        do__no_chain_fn(do__f_forbidden_err);
    case TI_FN_INDEX_ERR:
        do__no_chain_fn(do__f_index_err);
    case TI_FN_BAD_DATA_ERR:
        do__no_chain_fn(do__f_bad_data_err);
    case TI_FN_SYNTAX_ERR:
        do__no_chain_fn(do__f_syntax_err);
    case TI_FN_NODE_ERR:
        do__no_chain_fn(do__f_node_err);
    case TI_FN_ASSERT_ERR:
        do__no_chain_fn(do__f_assert_err);
    }

    if (is_chained)
    {
        ti_val_t * val = ti_query_val_pop(query);
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `%.*s`",
                ti_val_str(val),
                fname->len,
                fname->str);
        ti_val_drop(val);
    }
    else
    {
        ex_set(e, EX_INDEX_ERROR,
                "function `%.*s` is undefined",
                fname->len,
                fname->str);
    }
    return e->nr;

no_thingsdb_or_collection_scope:
    ex_set(e, EX_INDEX_ERROR,
            "function `%.*s` is undefined in the `%s` scope; "
            "You might want to query the `thingsdb` or a `collection` scope?",
            fname->len,
            fname->str,
            ti_query_scope_name(query));
    return e->nr;

no_node_scope:
    ex_set(e, EX_INDEX_ERROR,
            "function `%.*s` is undefined in the `%s` scope; "
            "You might want to query the `node` scope?",
            fname->len,
            fname->str,
            ti_query_scope_name(query));
    return e->nr;

no_thingsdb_scope:
    ex_set(e, EX_INDEX_ERROR,
            "function `%.*s` is undefined in the `%s` scope; "
            "You might want to query the `thingsdb` scope?",
            fname->len,
            fname->str,
            ti_query_scope_name(query));
    return e->nr;

no_collection_scope:
    ex_set(e, EX_INDEX_ERROR,
            "function `%.*s` is undefined in the `%s` scope; "
            "You might want to query a `collection` scope?",
            fname->len,
            fname->str,
            ti_query_scope_name(query));
    return e->nr;
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

static ti_prop_t * do__get_prop(ti_thing_t * thing, cleri_node_t * nd, ex_t * e)
{
    ti_name_t * name;
    ti_prop_t * prop;

    name = ti_names_weak_get(nd->str, nd->len);
    prop = name ? ti_thing_prop_weak_get(thing, name) : NULL;

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

    if (!ti_val_isthing(query->rval))
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                ti_val_str((ti_val_t *) thing));
        return e->nr;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, scope_nd, e))
        goto done;

    if (assign_nd->len == 2)
    {
        prop = do__get_prop(thing, name_nd, e);
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

        if (thing->flags & TI_VFLAG_LOCK)
        {
            ex_set_err(e, EX_BAD_DATA,
                "cannot assign properties while "TI_THING_ID" is in use",
                thing->id);
            goto done;
        }

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

        prop = ti_thing_prop_set(thing, name, query->rval);
        if (!prop)
        {
            ti_name_drop(name);
            goto alloc_err;
        }
    }
    ti_incref(prop->val);

    if (thing->id)
    {
        assert (query->target);  /* only in a collection scope */
        task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto done;

        if (ti_task_add_assign(task, prop->name, prop->val))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    goto done;

alloc_err:
    ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__block(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_BLOCK);

    cleri_children_t * child, * seqchild;

    seqchild = nd                       /* <{ comment, list s, [deep] }> */
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

    /* optional deep */
    ti_do_may_set_deep(&query->syntax.deep, seqchild->next, e);

    return e->nr;
}

static int do__index(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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

        if (ti_do_scope(query, node, e))
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
                ex_set_mem(e);
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

static int do__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);
    assert (query->rval);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* function, assignment,
                                               name */
    cleri_node_t * index_node = child->next->node;

    int chainn = query->chained->n;

    child = child->next->next;          /* set to chain child (or NULL) */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        if (do__function(query, node, true  /* is_chained */, e))
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

        if (!ti_val_isthing(query->rval))
        {
            ex_set(e, EX_BAD_DATA, "type `%s` has no properties",
                    ti_val_str((ti_val_t *) thing));
            return e->nr;
        }

        thing = (ti_thing_t *) query->rval;

        prop = do__get_prop(thing, node, e);
        if (!prop)
            return e->nr;

        if (thing->id && (index_node->children || child))
        {
            if (ti_chained_append(query->chained, thing, prop->name))
            {
                ex_set_mem(e);
                return e->nr;
            }
        }

        query->rval = prop->val;
        ti_incref(query->rval);
        ti_val_drop((ti_val_t *) thing);

        break;
    }
    default:
        assert (0);  /* all possible should be handled */
        goto done;
    }

    if (index_node->children && do__index(query, index_node, e))
        goto done;

    if (!child)
        goto done;

    (void) do__chain(query, child->node, e);

done:
    ti_chained_leave(query->chained, chainn);
    return e->nr;
}

static int do__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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
        return ti_do_scope(query, nd, e);

    gid = nd->children->next->node->children->node->cl_obj->gid;

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
            node->data = (ti_val_t *) ti_closure_from_node(nd);
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
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    int i
        = langdef_nd_match_str(nd, "READ")
        ? TI_AUTH_READ
        : langdef_nd_match_str(nd, "MODIFY")
        ? TI_AUTH_MODIFY
        : langdef_nd_match_str(nd, "WATCH")
        ? TI_AUTH_WATCH
        : langdef_nd_match_str(nd, "CALL")
        ? TI_AUTH_CALL
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

static int do__scope_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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

static ti_prop_t * do__get_var(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_name_t * name;
    ti_prop_t * prop;

    name = ti_names_weak_get(nd->str, nd->len);
    prop = name ? ti_query_var_get(query, name) : NULL;

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
    assert (nd->len >= 2);
    assert (ti_name_is_valid_strn(nd->str + 1, nd->len - 1));
    assert (query->rval == NULL);
    assert (ti_chained_get(query->chained) == NULL);

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
    assert (ti_chained_get(query->chained) == NULL);

    ti_name_t * name;
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
        prop = do__get_var(query, nd, e);
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
     * We must make variable assignable because if we for example later `push`
     * to an assigned array then we require a copy.
     */
    if (ti_val_make_assignable(&query->rval, e))
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
    uint32_t current_varn;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd         /* sequence */
            ->children;                             /* first child, not */

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* immutable, function,
                                               assignment, name, thing,
                                               array, compare, closure */

    /* make sure the current chain points to NULL */
    ti_chained_null(query->chained);

    current_varn = query->vars->n;

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        if (do__array(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_CHAIN:
        query->rval = query->root;
        ti_incref(query->rval);
        if (do__chain(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_BLOCK:
        if (do__block(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_FUNCTION:
        if (do__function(query, node, false /* is_chained */, e))
            goto on_error;
        break;
    case CLERI_GID_IMMUTABLE:
        if (do__immutable(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (do__operations(query, node->children->next->node, e))
            goto on_error;

        if (node->children->next->next->next)   /* optional (sequence) */
        {
            _Bool bool_ = ti_val_as_bool(query->rval);
            ti_val_drop(query->rval);
            query->rval = NULL;
            node = node->children->next->next->next->node;  /* sequence */
            if (bool_)
            {
                if (ti_do_scope(query, node->children->next->node, e))
                    goto on_error;
            }
            else if (node->children->next->next) /* else case */
            {
                node = node->children->next->next->node;  /* else sequence */
                if (ti_do_scope(query, node->children->next->node, e))
                    goto on_error;
            }
            else  /* no else case, just nil */
            {
                query->rval = (ti_val_t *) ti_nil_get();
            }
        }
        break;
    case CLERI_GID_THING:
        if (do__scope_thing(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_VAR:
        if (do__var(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_VAR_ASSIGN:
        if (do__var_assign(query, node, e))
            goto on_error;
        break;

    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    node = child->node;

    /* handle index */
    if (node->children && do__index(query, node, e))
        goto on_error;

    /* chain */
    if (child->next && do__chain(query, child->next->node, e))
        goto on_error;

//    if (!query->rval)
//    {
//        /* The scope value must exist since only `TMP` and `NAME` are not
//         * setting `query->rval` and they both set the scope value.
//         */
//        assert (query->scope->val);
//
//        query->rval = query->scope->val;
//        ti_incref(query->rval);
//    }

    if (nots)
    {
        _Bool b = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_vbool_get((nots & 1) ^ b);
    }

on_error:
    while (query->vars->n > current_varn)
        ti_prop_destroy(vec_pop(query->vars));
    return e->nr;
}

void ti__do_may_set_deep_(uint8_t * deep, cleri_children_t * child, ex_t * e)
{
    int64_t deepi;
    assert (child);
    assert (child->node->cl_obj->gid == CLERI_GID_DEEPHINT);
    deepi = strx_to_int64(child->node->children->next->node->str);
    if (errno == ERANGE)
        deepi = -1;

    if (deepi < 0 || deepi > DO__MAX_DEEP_HINT)
    {
        ex_set(e, EX_BAD_DATA,
                "expecting a `deep` value between 0 and %d, got "PRId64,
                DO__MAX_DEEP_HINT, deepi);
        return;
    }

    *deep = (uint8_t) deepi;
}
