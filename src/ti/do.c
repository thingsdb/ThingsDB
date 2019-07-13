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
#include <ti/fn/fndeltoken.h>
#include <ti/fn/fndeluser.h>
#include <ti/fn/fnendswith.h>
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
#include <ti/fn/fnprocedureinfo.h>
#include <ti/fn/fnproceduresinfo.h>
#include <ti/fn/fnpush.h>
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
        if (is_chained)
            break;
        return do__f_array(query, params, e);
    case TI_FN_ASSERT:
        if (is_chained)
            break;
        return do__f_assert(query, params, e);
    case TI_FN_BLOB:
        do__collection_fn(do__f_blob);
    case TI_FN_BOOL:
        if (is_chained)
            break;
        return do__f_bool(query, params, e);
    case TI_FN_CONTAINS:
        return do__f_contains(query, params, e);
    case TI_FN_ENDSWITH:
        return do__f_endswith(query, params, e);
    case TI_FN_DEL:
        return do__f_del(query, params, e);
    case TI_FN_FILTER:
        return do__f_filter(query, params, e);
    case TI_FN_FIND:
        return do__f_find(query, params, e);
    case TI_FN_FINDINDEX:
        return do__f_findindex(query, params, e);
    case TI_FN_FLOAT:
        if (is_chained)
            break;
        return do__f_float(query, params, e);
    case TI_FN_HAS:
        return do__f_has(query, params, e);
    case TI_FN_HASPROP:
        return do__f_hasprop(query, params, e);
    case TI_FN_ID:
        return do__f_id(query, params, e);
    case TI_FN_INDEXOF:
        return do__f_indexof(query, params, e);
    case TI_FN_INT:
        if (is_chained)
            break;
        return do__f_int(query, params, e);
    case TI_FN_ISARRAY:
        if (is_chained)
            break;
        return do__f_isarray(query, params, e);
    case TI_FN_ISASCII:
        if (is_chained)
            break;
        return do__f_isascii(query, params, e);
    case TI_FN_ISBOOL:
        if (is_chained)
            break;
        return do__f_isbool(query, params, e);
    case TI_FN_ISFLOAT:
        if (is_chained)
            break;
        return do__f_isfloat(query, params, e);
    case TI_FN_ISINF:
        if (is_chained)
            break;
        return do__f_isinf(query, params, e);
    case TI_FN_ISINT:
        if (is_chained)
            break;
        return do__f_isint(query, params, e);
    case TI_FN_ISLIST:
        if (is_chained)
            break;
        return do__f_islist(query, params, e);
    case TI_FN_ISNAN:
        if (is_chained)
            break;
        return do__f_isnan(query, params, e);
    case TI_FN_ISNIL:
        if (is_chained)
            break;
        return do__f_isnil(query, params, e);
    case TI_FN_ISRAW:
        if (is_chained)
            break;
        return do__f_israw(query, params, e);
    case TI_FN_ISSET:
        if (is_chained)
            break;
        return do__f_isset(query, params, e);
    case TI_FN_ISTHING:
        if (is_chained)
            break;
        return do__f_isthing(query, params, e);
    case TI_FN_ISTUPLE:
        if (is_chained)
            break;
        return do__f_istuple(query, params, e);
    case TI_FN_ISSTR:
    case TI_FN_ISUTF8:
        if (is_chained)
            break;
        return do__f_isutf8(query, params, e);
    case TI_FN_LEN:
        return do__f_len(query, params, e);
    case TI_FN_LOWER:
        return do__f_lower(query, params, e);
    case TI_FN_MAP:
        return do__f_map(query, params, e);
    case TI_FN_NOW:
        if (is_chained)
            break;
        return q__f_now(query, params, e);
    case TI_FN_POP:
        return do__f_pop(query, params, e);
    case TI_FN_PUSH:
        return do__f_push(query, params, e);
    case TI_FN_REFS:
        if (is_chained)
            break;
        return do__f_refs(query, params, e);
    case TI_FN_REMOVE:
        return do__f_remove(query, params, e);
    case TI_FN_RENAME:
        return do__f_rename(query, params, e);
    case TI_FN_SET:
        if (is_chained)
            break;
        return do__f_set(query, params, e);
    case TI_FN_SPLICE:
        return do__f_splice(query, params, e);
    case TI_FN_STARTSWITH:
        return do__f_startswith(query, params, e);
    case TI_FN_STR:
        if (is_chained)
            break;
        return do__f_str(query, params, e);
    case TI_FN_T:
        do__collection_fn(do__f_t);
    case TI_FN_TEST:
        return do__f_test(query, params, e);
    case TI_FN_TRY:
        if (is_chained)
            break;
        return do__f_try(query, params, e);
    case TI_FN_TYPE:
        if (is_chained)
            break;
        return do__f_type(query, params, e);
    case TI_FN_UPPER:
        return do__f_upper(query, params, e);

    /* both thingsdb and collection scope */
    case TI_FN_CALL:
        do__thingsdb_or_collection_fn(do__f_call);
    case TI_FN_CALLE:
        do__thingsdb_or_collection_fn(do__f_calle);
    case TI_FN_NEW_PROCEDURE:
        do__thingsdb_or_collection_fn(do__f_new_procedure);
    case TI_FN_PROCEDURE_DEF:
        do__thingsdb_or_collection_fn(do__f_procedure_def);
    case TI_FN_PROCEDURE_INFO:
        do__thingsdb_or_collection_fn(do__f_procedure_info);
    case TI_FN_PROCEDURES_INFO:
        do__thingsdb_or_collection_fn(do__f_procedures_info);

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
        ex_set_alloc(e);
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

static int do__assignment(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);

    ti_thing_t * thing;
    ti_name_t * name;
    ti_task_t * task = NULL;
    ti_val_t * left_val = NULL;     /* assign to prevent warning */
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

    if (ti_do_scope(query, scope_nd, e))
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
        assert (query->target);  /* only in a collection scope */
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

static int do__chain_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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

static int do__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* function, assignment,
                                               name */

    /* In case the query value is a `thing` without a scope of it's own,
     * a scope must be created and this seems to be the right place since
     * then the functions, assignments and chains can use the correct scope.
     */
    ti_thing_t * thing = (ti_thing_t *) (query->rval
            ? (ti_val_is_thing(query->rval)
                    ? query->rval : NULL)
            : (query->scope->val && ti_val_is_thing(query->scope->val)
                    ? query->scope->val : NULL));

    if (thing)
    {
        /* the scope value is a thing without an empty scope */
        ti_scope_t * tmp_scope = ti_scope_enter(query->scope, thing);
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
        if (do__function(query, node, true, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (do__assignment(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (do__chain_name(query, node, e))
            return e->nr;
        break;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    node = child->node;

    if (node->children && do__index(query, node, e))
        return e->nr;

    child = child->next;
    if (!child)
        return e->nr;

    (void) do__chain(query, child->node, e);
    return e->nr;
}

static int do__closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CLOSURE);

    query->rval = (ti_val_t *) ti_closure_from_node(nd);
    if (!query->rval)
        ex_set_alloc(e);

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

static int do__primitives(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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
                ex_set_alloc(e);
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
                ex_set_alloc(e);
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
                "`%.*s` is undefined",
                (int) nd->len, nd->str);
    }
    else
    {
        query->rval = (ti_val_t *) ti_vint_create(i);
        if (!query->rval)
            ex_set_alloc(e);
    }

    return e->nr;
}

/* changes scope->name and/or scope->thing */
static int do__scope_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));
    assert (query->rval == NULL);
    assert (query->scope->thing == query->root);

    ti_name_t * name;
    ti_val_t * val;

    name = ti_names_weak_get(nd->str, nd->len);

    /* first try to find the name in a local scopes */
    val = name ? ti_scope_find_local_val(query->scope, name) : NULL;

    /* next, try the root in case the `name` is not found */
    if (!val && name)
        val = ti_thing_prop_weak_get(query->root, name);

    if (!val)
    {
        if (do__no_collection_scope(query))
            return do__fixed_name(query, nd, e);

        ex_set(e, EX_INDEX_ERROR,
                "`%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    ti_scope_set_name_val(query->scope, name, val);

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
static int do__tmp(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_TMP);
    assert (nd->len >= 2);
    assert (ti_name_is_valid_strn(nd->str + 1, nd->len - 1));
    assert (query->rval == NULL);
    assert (query->scope->thing == query->root);

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

static int do__tmp_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_TMP_ASSIGN);
    assert (query->scope->thing == query->root);

    ti_name_t * name;
    ti_prop_t * prop = NULL;     /* assign to prevent warning */
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    /* should also work in chain because then scope->local must be NULL */
    if (    ti_scope_has_local_name(query->scope, name) ||
            ti_scope_in_use_name(query->scope, query->root, name))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot assign a new value to `%.*s` while the variable is in use",
            (int) name->n, name->str);
        goto failed;
    }

    prop = ti_query_tmpprop_get(query, name);

    if (prop)
    {
        ti_decref(name);  /* we already have a reference to name */
    }
    else if (assign_nd->len == 2)
    {
        ex_set(e, EX_INDEX_ERROR,
                "`%.*s` is undefined", (int) name->n, name->str);
        goto failed;
    }

    if (ti_do_scope(query, scope_nd, e))
        goto failed;

    assert (query->rval);

    if (prop)
    {
        if (    assign_nd->len == 2 &&
                ti_opr_a_to_b(prop->val, assign_nd, &query->rval, e))
            goto failed;
        ti_val_drop(prop->val);
        prop->val = query->rval;
    }
    else
    {
        assert (assign_nd->len == 1);
        prop = ti_prop_create(name, query->rval);
        if (!prop)
            goto alloc_err;

        if (!query->tmpvars)
        {
            query->tmpvars = vec_new(1);
            if (!query->tmpvars)
                goto alloc_err_with_prop;
            VEC_push(query->tmpvars, prop);
        }
        else if (vec_push(&query->tmpvars, prop))
            goto alloc_err_with_prop;
    }

    ti_incref(query->rval);
    return e->nr;

alloc_err_with_prop:
    /* prop->name will be dropped and prop->val is still on query->rval */
    free(prop);

alloc_err:
    ex_set_alloc(e);

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
    query->scope = ti_scope_enter(current_scope, query->root);
    if (!query->scope)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        if (do__array(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_CLOSURE:
        if (do__closure(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (do__no_collection_scope(query))
        {
            ex_set(e, EX_BAD_DATA,
                    "assignments are not supported in the `%s` scope",
                    ti_query_scope_name(query));
            goto on_error;
        }
        if (do__assignment(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_FUNCTION:
        if (do__function(query, node, false, e))
            goto on_error;
        break;
    case CLERI_GID_NAME:
        if (do__scope_name(query, node, e))
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
            if (ti_do_scope(
                    query,
                    bool_
                        ? node->children->next->node        /* scope, true */
                        : node->children->next->next->next->node, /* false */
                    e))
                goto on_error;
        }
        break;
    case CLERI_GID_PRIMITIVES:
        if (do__primitives(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_THING:
        if (do__scope_thing(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_TMP:
        if (do__tmp(query, node, e))
            goto on_error;
        break;
    case CLERI_GID_TMP_ASSIGN:
        if (do__tmp_assign(query, node, e))
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

on_error:
    ti_scope_leave(&query->scope, current_scope);
    return e->nr;
}

int ti_do_optscope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->rval == NULL);
    if (!nd)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }
    return ti_do_scope(query, nd, e);
}
