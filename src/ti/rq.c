/*
 * ti/rq.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/collections.h>
#include <ti/nil.h>
#include <ti/opr.h>
#include <ti/regex.h>
#include <ti/rq.h>
#include <ti/task.h>
#include <ti/users.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/cryptx.h>
#include <util/strx.h>
#include <uv.h>

static int rq__f_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_collections(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_counters(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_del_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_del_user(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_grant(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_new_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_new_node(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_new_user(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_node(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_nodes(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_pop_node(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_rename_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_rename_user(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_reset_counters(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_revoke(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_set_loglevel(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_set_password(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_set_quota(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_set_zone(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_shutdown(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_user(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__f_users(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__function(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__name(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__primitives(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int rq__scope(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static _Bool rq__is_not_node(ti_query_t * q, cleri_node_t * n, ex_t * e);
static _Bool rq__is_not_thingsdb(ti_query_t * q, cleri_node_t * n, ex_t * e);

int ti_rq_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    query->flags &= ~TI_QUERY_FLAG_ROOT_NESTED;
    return rq__scope(query, nd, e);
}

static int rq__f_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_collection_t * collection;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `collection` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    assert (collection);


    ti_val_drop(query->rval);
    query->rval = ti_collection_as_qpval(collection);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int rq__f_collections(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `collections` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_collections_as_qpval();
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}


static int rq__f_counters(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `counters` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_counters_as_qpval();
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int rq__f_del_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    uint64_t collection_id;
    ti_collection_t * collection;
    ti_task_t * task;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del_collection` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    collection_id = collection->root->id;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_collection(task, collection_id))
        ex_set_alloc(e);  /* task cleanup is not required */
    else
        (void) ti_collections_del_collection(collection_id);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_del_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * ruser;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del_user` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `del_user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) ruser->n,
                (char *) ruser->data);
        return e->nr;
    }

    if (query->stream->via.user == user)
    {
        ex_set(e, EX_BAD_DATA,
                "it is not possible to delete your own user account");
        return e->nr;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_user(task, user))
        ex_set_alloc(e);  /* task cleanup is not required */

    /* this will remove the user so it cannot be used after here */
    ti_users_del_user(user);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_grant(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    int n;
    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * ruser;
    uint64_t mask, target_id;
    vec_t ** access_;

    n = langdef_nd_n_function_params(nd);
    if (n != 3)
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` requires 3 arguments but %d %s given, "
            "see: "TI_DOCS"#grant",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    /* grant target, target maybe NULL for root */
    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &target_id);
    if (e->nr)
        return e->nr;

    /* check for privileges */
    if (ti_access_check_err(
            *access_,
            query->stream->via.user, TI_AUTH_GRANT, e))
        return e->nr;

    /* grant user */
    ti_val_drop(query->rval);
    query->rval = NULL;
    if (rq__scope(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`, see: "TI_DOCS"#grant",
            ti_val_str(query->rval));
        return e->nr;
    }

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) ruser->n,
                (char *) ruser->data);
        return e->nr;
    }

    /* grant mask */
    ti_val_drop(query->rval);
    query->rval = NULL;
    if (rq__scope(query, nd->children->next->next->next->next->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` expects argument 3 to be of "
            "type `"TI_VAL_INT_S"` but got `%s`, see: "TI_DOCS"#grant",
            ti_val_str(query->rval));
        return e->nr;
    }

    mask = (uint64_t) ((ti_vint_t *) query->rval)->int_;

    /* make sure READ when MODIFY and MODIFY when GRANT */
    if (mask & TI_AUTH_GRANT)
        mask |= TI_AUTH_READ|TI_AUTH_MODIFY;
    else if (mask & TI_AUTH_MODIFY)
        mask |= TI_AUTH_READ;

    if (ti_access_grant(access_, user, mask))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_grant(task, target_id, user, mask))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_new_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_raw_t * rname;
    ti_collection_t * collection;
    ti_task_t * task;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `new_collection` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_collection` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;

    collection = ti_collections_create_collection(
            0,
            (const char *) rname->data,
            rname->n,
            query->stream->via.user,
            e);
    if (!collection)
        goto finish;

    task = ti_task_get_task(query->ev, ti()->thing0, e);

    if (!task)
        goto finish;

    if (ti_task_add_new_collection(task, collection, query->stream->via.user))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) collection->root->id);
    if (!query->rval)
        ex_set_alloc(e);

finish:
    return e->nr;
}

static int rq__f_new_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    char salt[CRYPTX_SALT_SZ];
    char encrypted[CRYPTX_SZ];
    char * secret;
    ti_node_t * node;
    ti_raw_t * rsecret;
    ti_raw_t * raddr;
    ti_task_t * task;
    cleri_children_t * child;
    struct in_addr sa;
    struct in6_addr sa6;
    struct sockaddr_storage addr;
    char * addrstr;
    int port, n = langdef_nd_n_function_params(nd);

    if (n < 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` requires at least 2 arguments but %d %s given",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }
    else if (n > 3)
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` takes at most 3 arguments but %d were given",
            n);
        return e->nr;
    }

    child = nd->children;
    if (rq__scope(query, child->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    rsecret = (ti_raw_t *) query->rval;
    if (!rsecret->n || !strx_is_graphn((char *) rsecret->data, rsecret->n))
    {
        ex_set(e, EX_BAD_DATA,
            "a `secret` is required "
            "and should only contain graphical characters");
        return e->nr;
    }

    secret = ti_raw_to_str(rsecret);
    if (!secret)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;
    child = child->next->next;

    if (rq__scope(query, child->node, e))
        goto fail0;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` expects argument 2 to be of type `"TI_VAL_RAW_S"` "
            "but got `%s`",
            ti_val_str(query->rval));
        goto fail0;
    }

    raddr = (ti_raw_t *) query->rval;
    if (raddr->n >= INET6_ADDRSTRLEN)
    {
        ex_set(e, EX_BAD_DATA, "invalid IPv4/6 address: `%.*s`",
                (int) raddr->n,
                (char *) raddr->data);
        goto fail0;
    }

    addrstr = ti_raw_to_str(raddr);
    if (!addrstr)
    {
        ex_set_alloc(e);
        goto fail0;
    }

    if (n == 3)
    {
        int64_t iport;
        ti_val_drop(query->rval);
        query->rval = NULL;
        child = child->next->next;

        /* Read the port number from arguments */
        if (rq__scope(query, child->node, e))
            goto fail1;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `new_node` expects argument 3 to be of "
                "type `"TI_VAL_INT_S"` but got `%s`",
                ti_val_str(query->rval));
            goto fail1;
        }

        iport = ((ti_vint_t *) query->rval)->int_;
        if (iport < 1<<0 || iport >= 1<<16)
        {
            ex_set(e, EX_BAD_DATA,
                "`port` should be an integer value between 1 and 65535, "
                "got %"PRId64,
                iport);
            goto fail1;
        }
        port = (int) iport;
    }
    else
    {
        /* Use default port number as no port is given as argument */
        port = TI_DEFAULT_NODE_PORT;
    }

    if (inet_pton(AF_INET, addrstr, &sa))  /* try IPv4 */
    {
        if (uv_ip4_addr(addrstr, port, (struct sockaddr_in *) &addr))
        {
            ex_set(e, EX_INTERNAL,
                    "cannot create IPv4 address from `%s:%d`",
                    addrstr, port);
            goto fail1;
        }
    }
    else if (inet_pton(AF_INET6, addrstr, &sa6))  /* try IPv6 */
    {
        if (uv_ip6_addr(addrstr, port, (struct sockaddr_in6 *) &addr))
        {
            ex_set(e, EX_INTERNAL,
                    "cannot create IPv6 address from `[%s]:%d`",
                    addrstr, port);
            goto fail1;
        }
    }
    else
    {
        ex_set(e, EX_BAD_DATA, "invalid IPv4/6 address: `%s`", addrstr);
        goto fail1;
    }

    cryptx_gen_salt(salt);
    cryptx(secret, salt, encrypted);

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto fail1;

    node = ti_nodes_new_node(0, port, addrstr, encrypted);
    if (!node)
    {
        ex_set_alloc(e);
        goto fail1;
    }

    if (ti_task_add_new_node(task, node))
        ex_set_alloc(e);  /* task cleanup is not required */

    (void) ti_save();

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) node->id);
    if (!query->rval)
        ex_set_alloc(e);

fail1:
    free(addrstr);
fail0:
    free(secret);
    return e->nr;
}

static int rq__f_new_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    char * passstr = NULL;
    int n;
    ti_user_t * nuser;
    ti_raw_t * rname;
    ti_task_t * task;

    n = langdef_nd_n_function_params(nd);
    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_user` requires 2 arguments but %d %s given",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (rq__scope(query, nd->children->next->next->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_user` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        goto done;
    }

    passstr = ti_raw_to_str((ti_raw_t *) query->rval);
    if (!passstr)
    {
        ex_set_alloc(e);
        goto done;
    }

    nuser = ti_users_new_user(
            (const char *) rname->data, rname->n,
            passstr, e);

    if (!nuser)
        goto done;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto done;

    if (ti_task_add_new_user(task, nuser))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) nuser->id);
    if (!query->rval)
        ex_set_alloc(e);

done:
    free(passstr);
    ti_val_drop((ti_val_t *) rname);

    return e->nr;
}

static int rq__f_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `node` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_node_as_qpval();
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int rq__f_nodes(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `nodes` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_nodes_info_as_qpval();
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int rq__f_pop_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_node_t * node;
    uint8_t node_id;
    ti_task_t * task;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `pop_node` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    node = vec_last(ti()->nodes->vec);
    assert (node);

    if (node->status >= TI_NODE_STAT_SYNCHRONIZING)
    {
        ex_set(e, EX_NODE_ERROR,
                TI_NODE_ID" (%s) still has an active connection to ThingsDB",
                node->id, ti_stream_name(node->stream));
        return e->nr;
    }

    assert (node != ti()->node);
    node_id = node->id;

    ti_nodes_pop_node();

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_pop_node(task, node_id))
        ex_set_alloc(e);  /* task cleanup is not required */

    return e->nr;
}

static int rq__f_rename_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    int n;
    ti_task_t * task;
    ti_collection_t * collection;

    n = langdef_nd_n_function_params(nd);
    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_collection` requires 2 arguments "
            "but %d %s given",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    assert (e->nr == 0);
    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;
    assert (collection);

    if (rq__scope(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_collection` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    if (ti_collection_rename(collection, (ti_raw_t *) query->rval, e))
        return e->nr;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_rename_collection(task, collection))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_rename_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    int n;
    ti_task_t * task;
    ti_user_t * user;
    ti_raw_t * rname;

    n = langdef_nd_n_function_params(nd);
    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_user` requires 2 arguments but %d %s given",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) rname->data, rname->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) rname->n,
                (char *) rname->data);
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;
    if (rq__scope(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_user` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    if (!rname)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    if (ti_user_rename(user, rname, e))
        return e->nr;

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_rename_user(task, user))
        ex_set_alloc(e);  /* task cleanup is not required */

    return e->nr;
}

static int rq__f_reset_counters(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->rval == NULL);

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->stream->via.user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `reset_counters` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    ti_counters_reset();

    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_revoke(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    int n;
    ti_user_t * user;
    ti_raw_t * uname;
    ti_task_t * task;
    uint64_t mask, target_id;
    vec_t ** access_;

    n = langdef_nd_n_function_params(nd);
    if (n != 3)
    {
        ex_set(e, EX_BAD_DATA,
            "function `revoke` requires 3 arguments but %d %s given, "
            "see: "TI_DOCS"#grant",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    /* revoke target, target maybe NULL for root */
    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &target_id);
    if (e->nr)
        return e->nr;

    /* check for privileges */
    if (ti_access_check_err(
            *access_,
            query->stream->via.user, TI_AUTH_GRANT, e))
        return e->nr;

    /* revoke user */
    ti_val_drop(query->rval);
    query->rval = NULL;
    if (rq__scope(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `revoke` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`, see: "TI_DOCS"#grant",
            ti_val_str(query->rval));
        return e->nr;
    }

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) uname->n,
                (char *) uname->data);
        return e->nr;
    }

    /* revoke mask */
    ti_val_drop(query->rval);
    query->rval = NULL;
    if (rq__scope(query, nd->children->next->next->next->next->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `revoke` expects argument 3 to be of "
            "type `"TI_VAL_INT_S"` but got `%s`, see: "TI_DOCS"#grant",
            ti_val_str(query->rval));
        return e->nr;
    }

    mask = (uint64_t) ((ti_vint_t *) query->rval)->int_;

    /* make sure READ when MODIFY and MODIFY when GRANT */
    if (mask & TI_AUTH_READ)
        mask |= TI_AUTH_MODIFY|TI_AUTH_GRANT;
    else if (mask & TI_AUTH_MODIFY)
        mask |= TI_AUTH_GRANT;

    if (query->stream->via.user == user && (mask & TI_AUTH_GRANT))
    {
        ex_set(e, EX_BAD_DATA,
                "it is not possible to revoke your own `GRANT` privileges");
        return e->nr;
    }

    ti_access_revoke(*access_, user, mask);

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_revoke(task, target_id, user, mask))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_set_loglevel(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->rval == NULL);
    int log_level;
    int64_t ilog;

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->stream->via.user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `set_loglevel` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_loglevel` expects argument 1 to be of "
            "type `"TI_VAL_INT_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    ilog = ((ti_vint_t *) query->rval)->int_;

    log_level = ilog < LOGGER_DEBUG
            ? LOGGER_DEBUG
            : ilog > LOGGER_CRITICAL
            ? LOGGER_CRITICAL
            : ilog;

    logger_set_level(log_level);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_set_password(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    int n;
    char * passstr = NULL;
    ti_raw_t * uname;
    ti_user_t * user;
    ti_task_t * task;
    n = langdef_nd_n_function_params(nd);

    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_password` requires 2 arguments but %d %s given",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_password` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) uname->n,
                (char *) uname->data);
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (rq__scope(query, nd->children->next->next->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_password` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        goto done;
    }

    passstr = ti_raw_to_str((ti_raw_t *) query->rval);
    if (!passstr)
    {
        ex_set_alloc(e);
        goto done;
    }

    if (!ti_user_pass_check(passstr, e))
        goto done;

    if (ti_user_set_pass(user, passstr))
    {
        ex_set_alloc(e);
        goto done;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto done;

    if (ti_task_add_set_password(task, user))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    free(passstr);
    return e->nr;
}

static int rq__f_set_quota(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (query->stream->via.user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_quota_enum_t qtp;
    size_t quota;
    ti_raw_t * rquota;
    ti_collection_t * collection;
    ti_task_t * task;
    uint64_t collection_id;
    int n = langdef_nd_n_function_params(nd);

    if (n != 3)
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `quota` takes 3 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    collection_id = collection->root->id;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (rq__scope(query, nd->children->next->next->node, e))
        return e->nr;

    if (query->rval->tp != TI_VAL_RAW)
    {
        ex_set(e, EX_BAD_DATA,
            "function `quota` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    rquota = (ti_raw_t *) query->rval;
    qtp = ti_qouta_tp_from_strn((const char *) rquota->data, rquota->n, e);
    if (e->nr)
        return e->nr;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (rq__scope(query, nd->children->next->next->next->next->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval) && !ti_val_is_nil(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `quota` expects argument 3 to be of "
            "type `"TI_VAL_INT_S"` or "TI_VAL_NIL_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    quota = (size_t) (ti_val_is_nil(query->rval)
            ? TI_QUOTA_NOT_SET
            : ((ti_vint_t *) query->rval)->int_ < 0
            ? 0
            : ((ti_vint_t *) query->rval)->int_);

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_set_quota(task, collection_id, qtp, quota))
        ex_set_alloc(e);  /* task cleanup is not required */
    else
        ti_collection_set_quota(collection, qtp, quota);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_set_zone(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->rval == NULL);

    int64_t izone;
    uint8_t zone;

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->stream->via.user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `set_zone` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_zone` expects argument 1 to be of "
            "type `"TI_VAL_INT_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    izone = ((ti_vint_t *) query->rval)->int_;

    if (izone < 0 || izone > 0xff)
    {
        ex_set(e, EX_BAD_DATA,
            "`zone` should be an integer between 0 and 255, got %"PRId64,
            izone);
        return e->nr;
    }

    zone = (uint8_t) izone;

    ti_set_and_broadcast_node_zone(zone);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_shutdown(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->rval == NULL);

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->stream->via.user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `shutdown` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    ti_term(SIGINT);

    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int rq__f_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_user_t * user;
    ti_raw_t * uname;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `user` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got `%s`",
            ti_val_str(query->rval));
        return e->nr;
    }

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) uname->n,
                (char *) uname->data);
        return e->nr;
    }

    ti_val_drop(query->rval);

    query->rval = ti_user_as_qpval(user);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int rq__f_users(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `users` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_users_as_qpval();
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int rq__function(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));
    assert (query->rval == NULL);  /* NULL since we never get here nested */

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or name node */

    params = nd                             /* sequence */
            ->children->next->next->node;   /* list of scope (arguments) */

    /* a function has at least size 1 */
    switch (*fname->str)
    {
    case 'c':
        if (langdef_nd_match_str(fname, "collection"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_collection(query, params, e)
            );
        if (langdef_nd_match_str(fname, "collections"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_collections(query, params, e)
            );
        if (langdef_nd_match_str(fname, "counters"))
            return (
                rq__is_not_node(query, fname, e) ||
                rq__f_counters(query, params, e)
            );
        break;
    case 'd':
        if (langdef_nd_match_str(fname, "del_collection"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_del_collection(query, params, e)
            );
        if (langdef_nd_match_str(fname, "del_user"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_del_user(query, params, e)
            );
        break;
    case 'g':
        if (langdef_nd_match_str(fname, "grant"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_grant(query, params, e)
            );
        break;
    case 'n':
        if (langdef_nd_match_str(fname, "new_collection"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_new_collection(query, params, e)
            );
        if (langdef_nd_match_str(fname, "new_node"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_new_node(query, params, e)
            );
        if (langdef_nd_match_str(fname, "new_user"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_new_user(query, params, e)
            );
        if (langdef_nd_match_str(fname, "node"))
            return (
                rq__is_not_node(query, fname, e) ||
                rq__f_node(query, params, e)
            );
        if (langdef_nd_match_str(fname, "nodes"))
            return (
                rq__is_not_node(query, fname, e) ||
                rq__f_nodes(query, params, e)
            );
        break;
    case 'p':
        if (langdef_nd_match_str(fname, "pop_node"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_pop_node(query, params, e)
            );
        break;
    case 'r':
        if (langdef_nd_match_str(fname, "rename_collection"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_rename_collection(query, params, e)
            );
        if (langdef_nd_match_str(fname, "rename_user"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_rename_user(query, params, e)
            );
        if (langdef_nd_match_str(fname, "reset_counters"))
            return (
                rq__is_not_node(query, fname, e) ||
                rq__f_reset_counters(query, params, e)
            );
        if (langdef_nd_match_str(fname, "revoke"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_revoke(query, params, e)
            );
        break;
    case 's':
        if (langdef_nd_match_str(fname, "set_loglevel"))
            return (
                rq__is_not_node(query, fname, e) ||
                rq__f_set_loglevel(query, params, e)
            );
        if (langdef_nd_match_str(fname, "set_password"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_set_password(query, params, e)
            );
        if (langdef_nd_match_str(fname, "set_quota"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_set_quota(query, params, e)
            );
        if (langdef_nd_match_str(fname, "set_zone"))
            return (
                rq__is_not_node(query, fname, e) ||
                rq__f_set_zone(query, params, e)
            );
        if (langdef_nd_match_str(fname, "shutdown"))
            return (
                rq__is_not_node(query, fname, e) ||
                rq__f_shutdown(query, params, e)
            );
        break;
    case 'u':
        if (langdef_nd_match_str(fname, "user"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_user(query, params, e)
            );
        if (langdef_nd_match_str(fname, "users"))
            return (
                rq__is_not_thingsdb(query, fname, e) ||
                rq__f_users(query, params, e)
            );
        break;
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

    _Bool nested = query->flags & TI_QUERY_FLAG_ROOT_NESTED;
    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd         /* sequence */
                    ->children;                     /* first child, not */

    query->flags |= TI_QUERY_FLAG_ROOT_NESTED;

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
                    "root functions are not allowed as arguments");
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
    if (q->flags & TI_QUERY_FLAG_NODE)
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
    if (~q->flags & TI_QUERY_FLAG_NODE)
        return false;

    ex_set(e, EX_INDEX_ERROR,
            "`%.*s` is undefined in this `node` scope; "
            "You might want to query the `thingsdb` scope?",
            n->len,
            n->str);
    return true;
}
