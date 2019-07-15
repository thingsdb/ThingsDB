#include <ti/fn/fn.h>

#define NEW_NODE_DOC_ TI_SEE_DOC("#new_node")

static int do__f_new_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB);
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
    int port, nargs = langdef_nd_n_function_params(nd);

    if (nargs < 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` requires at least 2 arguments but %d %s given"
            NEW_NODE_DOC_, nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }
    else if (nargs > 3)
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` takes at most 3 arguments but %d were given"
            NEW_NODE_DOC_, nargs);
        return e->nr;
    }

    child = nd->children;
    if (ti_do_scope(query, child->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"NEW_NODE_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    rsecret = (ti_raw_t *) query->rval;
    if (!rsecret->n || !strx_is_graphn((char *) rsecret->data, rsecret->n))
    {
        ex_set(e, EX_BAD_DATA,
            "a `secret` is required "
            "and should only contain graphical characters"NEW_NODE_DOC_);
        return e->nr;
    }

    secret = ti_raw_to_str(rsecret);
    if (!secret)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;
    child = child->next->next;

    if (ti_do_scope(query, child->node, e))
        goto fail0;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_node` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"NEW_NODE_DOC_,
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
        ex_set_mem(e);
        goto fail0;
    }

    if (nargs == 3)
    {
        int64_t iport;
        ti_val_drop(query->rval);
        query->rval = NULL;
        child = child->next->next;

        /* Read the port number from arguments */
        if (ti_do_scope(query, child->node, e))
            goto fail1;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `new_node` expects argument 3 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"NEW_NODE_DOC_,
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
        ex_set_mem(e);
        goto fail1;
    }

    if (ti_task_add_new_node(task, node))
        ex_set_mem(e);  /* task cleanup is not required */

    query->ev->flags |= TI_EVENT_FLAG_SAVE;

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) node->id);
    if (!query->rval)
        ex_set_mem(e);

fail1:
    free(addrstr);
fail0:
    free(secret);
    return e->nr;
}
