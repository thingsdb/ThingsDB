/*
 * ti/rq.c
 */
#include <ti/rfn/fncollection.h>
#include <ti/rfn/fncollections.h>
#include <ti/rfn/fncounters.h>
#include <ti/rfn/fndelcollection.h>
#include <ti/rfn/fndeluser.h>
#include <ti/rfn/fngrant.h>
#include <ti/rfn/fnnewcollection.h>
#include <ti/rfn/fnnewnode.h>
#include <ti/rfn/fnnewuser.h>
#include <ti/rfn/fnnode.h>
#include <ti/rfn/fnnodes.h>
#include <ti/rfn/fnpopnode.h>
#include <ti/rfn/fnrenamecollection.h>
#include <ti/rfn/fnrenameuser.h>
#include <ti/rfn/fnreplacenode.h>
#include <ti/rfn/fnresetcounters.h>
#include <ti/rfn/fnrevoke.h>
#include <ti/rfn/fnsetloglevel.h>
#include <ti/rfn/fnsetpassword.h>
#include <ti/rfn/fnsetquota.h>
#include <ti/rfn/fnsetzone.h>
#include <ti/rfn/fnshutdown.h>
#include <ti/rfn/fnuser.h>
#include <ti/rfn/fnusers.h>

static int rq__function(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));
    assert (query->rval == NULL);  /* NULL since we never get here nested */

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node;       /* name node */

    params = nd                             /* sequence */
            ->children->next->next->node;   /* list of scope (arguments) */

    switch ((ti_fn_enum_t) ((uintptr_t) fname->data))
    {
    case TI_FN_0:
    case TI_FN_ADD:
    case TI_FN_ASSERT:
    case TI_FN_BLOB:
    case TI_FN_BOOL:
    case TI_FN_DEL:
    case TI_FN_ENDSWITH:
    case TI_FN_FILTER:
    case TI_FN_FIND:
    case TI_FN_FINDINDEX:
    case TI_FN_HAS:
    case TI_FN_HASPROP:
    case TI_FN_ID:
    case TI_FN_INDEXOF:
    case TI_FN_INT:
    case TI_FN_ISARRAY:
    case TI_FN_ISASCII:
    case TI_FN_ISBOOL:
    case TI_FN_ISFLOAT:
    case TI_FN_ISINF:
    case TI_FN_ISINT:
    case TI_FN_ISLIST:
    case TI_FN_ISNAN:
    case TI_FN_ISNIL:
    case TI_FN_ISRAW:
    case TI_FN_ISSTR:
    case TI_FN_ISTHING:
    case TI_FN_ISTUPLE:
    case TI_FN_ISUTF8:
    case TI_FN_LEN:
    case TI_FN_LOWER:
    case TI_FN_MAP:
    case TI_FN_NOW:
    case TI_FN_POP:
    case TI_FN_PUSH:
    case TI_FN_REFS:
    case TI_FN_REMOVE:
    case TI_FN_RENAME:
    case TI_FN_RET:
    case TI_FN_SET:
    case TI_FN_SPLICE:
    case TI_FN_STARTSWITH:
    case TI_FN_STR:
    case TI_FN_T:
    case TI_FN_TEST:
    case TI_FN_TRY:
    case TI_FN_UPPER:
        break;
    case TI_FN_COLLECTION:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_collection(query, params, e)
        );
    case TI_FN_COLLECTIONS:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_collections(query, params, e)
        );
    case TI_FN_COUNTERS:
        return (
            rq__is_not_node(query, fname, e) ||
            rq__f_counters(query, params, e)
        );
    case TI_FN_DEL_COLLECTION:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_del_collection(query, params, e)
        );
    case TI_FN_DEL_USER:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_del_user(query, params, e)
        );
    case TI_FN_GRANT:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_grant(query, params, e)
        );
    case TI_FN_NEW_COLLECTION:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_new_collection(query, params, e)
        );
    case TI_FN_NEW_NODE:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_new_node(query, params, e)
        );
    case TI_FN_NEW_USER:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_new_user(query, params, e)
        );
    case TI_FN_NODE:
        return (
            rq__is_not_node(query, fname, e) ||
            rq__f_node(query, params, e)
        );
    case TI_FN_NODES:
        return (
            rq__is_not_node(query, fname, e) ||
            rq__f_nodes(query, params, e)
        );
    case TI_FN_POP_NODE:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_pop_node(query, params, e)
        );
    case TI_FN_RENAME_COLLECTION:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_rename_collection(query, params, e)
        );
    case TI_FN_RENAME_USER:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_rename_user(query, params, e)
        );
    case TI_FN_REPLACE_NODE:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_replace_node(query, params, e)
        );
    case TI_FN_RESET_COUNTERS:
        return (
            rq__is_not_node(query, fname, e) ||
            rq__f_reset_counters(query, params, e)
        );
    case TI_FN_REVOKE:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_revoke(query, params, e)
        );
    case TI_FN_SET_PASSWORD:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_set_password(query, params, e)
        );
    case TI_FN_SET_LOGLEVEL:
        return (
            rq__is_not_node(query, fname, e) ||
            rq__f_set_loglevel(query, params, e)
        );
    case TI_FN_SET_QUOTA:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_set_quota(query, params, e)
        );
    case TI_FN_SET_ZONE:
        return (
            rq__is_not_node(query, fname, e) ||
            rq__f_set_zone(query, params, e)
        );
    case TI_FN_SHUTDOWN:
        return (
            rq__is_not_node(query, fname, e) ||
            rq__f_shutdown(query, params, e)
        );
    case TI_FN_USER:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_user(query, params, e)
        );
    case TI_FN_USERS:
        return (
            rq__is_not_thingsdb(query, fname, e) ||
            rq__f_users(query, params, e)
        );
    }

    ex_set(e, EX_INDEX_ERROR,
            "`%.*s` is undefined",
            fname->len,
            fname->str);

    return e->nr;
}

static int rq__name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    int flags
        = langdef_nd_match_str(nd, "READ")
        ? TI_AUTH_READ
        : langdef_nd_match_str(nd, "MODIFY")
        ? TI_AUTH_MODIFY
        : langdef_nd_match_str(nd, "WATCH")
        ? TI_AUTH_WATCH
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
        : 0;

    if (flags)
    {
        query->rval = (ti_val_t *) ti_vint_create(flags);
        if (!query->rval)
            ex_set_alloc(e);
    }
    else
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);

    return e->nr;
}

static int rq__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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
        return rq__scope(query, nd, e);

    gid = nd->children->next->node->children->node->cl_obj->gid;

    switch (gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_AND:
    case CLERI_GID_OPR3_BITWISE_XOR:
    case CLERI_GID_OPR4_BITWISE_OR:
    case CLERI_GID_OPR5_COMPARE:
        if (rq__operations(query, nd->children->node, e))
            return e->nr;
        a_val = query->rval;
        query->rval = NULL;
        if (rq__operations(query, nd->children->next->next->node, e))
            break;
        (void) ti_opr_a_to_b(a_val, nd->children->next->node, &query->rval, e);
        break;

    case CLERI_GID_OPR6_CMP_AND:
        if (    rq__operations(query, nd->children->node, e) ||
                !ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return rq__operations(query, nd->children->next->next->node, e);

    case CLERI_GID_OPR7_CMP_OR:
        if (    rq__operations(query, nd->children->node, e) ||
                ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return rq__operations(query, nd->children->next->next->node, e);

    default:
        assert (0);
    }

    ti_val_drop(a_val);
    return e->nr;
}

static int rq__primitives(ti_query_t * query, cleri_node_t * nd, ex_t * e)
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
        query->rval = (ti_val_t *) ti_vfloat_create(strx_to_double(node->str));
        if (!query->rval)
            ex_set_alloc(e);
        break;
    case CLERI_GID_T_INT:
        query->rval = (ti_val_t *) ti_vint_create(strx_to_int64(node->str));
        if (!query->rval)
            ex_set_alloc(e);
        if (errno == ERANGE)
            ex_set(e, EX_OVERFLOW, "integer overflow");
        break;
    case CLERI_GID_T_NIL:
        query->rval = (ti_val_t *) ti_nil_get();
        break;
    case CLERI_GID_T_REGEX:
        query->rval = (ti_val_t *) ti_regex_from_strn(node->str, node->len, e);
        break;
    case CLERI_GID_T_STRING:
        query->rval = (ti_val_t *) ti_raw_from_ti_string(node->str, node->len);
        if (!query->rval)
            ex_set_alloc(e);
        break;
    case CLERI_GID_T_TRUE:
        query->rval = (ti_val_t *) ti_vbool_get(true);
        break;
    }
    return e->nr;
}

static int rq__scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    _Bool nested = query->syntax.flags & TI_SYNTAX_FLAG_NESTED;
    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd         /* sequence */
                    ->children;                     /* first child, not */

    query->syntax.flags |= TI_SYNTAX_FLAG_NESTED;

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, closure */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        ex_set(e, EX_BAD_DATA, "arrays are not supported at root");
        return e->nr;
    case CLERI_GID_CLOSURE:
        ex_set(e, EX_BAD_DATA, "closure functions are not supported at root");
        return e->nr;
    case CLERI_GID_ASSIGNMENT:
        ex_set(e, EX_BAD_DATA, "assignments are not supported at root");
        return e->nr;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (rq__operations(query, node->children->next->node, e))
            return e->nr;

        if (node->children->next->next->next)               /* optional */
        {
            node = node->children->next->next->next->node   /* choice */
                   ->children->node;                        /* sequence */
            if (rq__scope(
                    query,
                    ti_val_as_bool(query->rval)
                        ? node->children->next->node        /* scope, true */
                        : node->children->next->next->next->node, /* false */
                    e))
                return e->nr;
        }
        break;
    case CLERI_GID_FUNCTION:
        if (nested)
        {
            ex_set(e, EX_BAD_DATA,
                "functions are not allowed as arguments in the current scope");
            return e->nr;
        }
        if (rq__function(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (rq__name(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_PRIMITIVES:
        if (rq__primitives(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_THING:
        ex_set(e, EX_BAD_DATA, "things are not supported at root");
        break;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;

    assert (child);

    node = child->node;
    assert (node->cl_obj->gid == CLERI_GID_INDEX);

    if (node->children)
    {
        ex_set(e, EX_BAD_DATA, "indexing is not supported at root");
        return e->nr;
    }

    if (child->next)
    {
        ex_set(e, EX_BAD_DATA, "chaining is not supported at root");
        return e->nr;
    }

    if (!query->rval)
        query->rval = (ti_val_t *) ti_nil_get();

    if (nots)
    {
        _Bool b = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_vbool_get((nots & 1) ^ b);
    }

    return e->nr;
}

static _Bool rq__is_not_node(ti_query_t * q, cleri_node_t * n, ex_t * e)
{
    if (q->syntax.flags & TI_SYNTAX_FLAG_NODE)
        return false;

    ex_set(e, EX_INDEX_ERROR,
            "`%.*s` is undefined in the `thingsdb` scope; "
            "You want to query a `node` scope?",
            n->len,
            n->str);
    return true;
}

static _Bool rq__is_not_thingsdb(ti_query_t * q, cleri_node_t * n, ex_t * e)
{
    if (~q->syntax.flags & TI_SYNTAX_FLAG_NODE)
        return false;

    ex_set(e, EX_INDEX_ERROR,
            "`%.*s` is undefined in the `node` scope; "
            "You might want to query the `thingsdb` scope?",
            n->len,
            n->str);
    return true;
}

int ti_rq_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    query->syntax.flags &= ~TI_SYNTAX_FLAG_NESTED;
    return rq__scope(query, nd, e);
}
