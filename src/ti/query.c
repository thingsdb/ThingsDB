/*
 * query.c
 */
#include <assert.h>
#include <ti/query.h>
#include <ti.h>
#include <ti/dbs.h>
#include <ti/res.h>
#include <ti/proto.h>
#include <stdlib.h>
#include <qpack.h>
#include <langdef/translate.h>
#include <langdef/nd.h>
#include <util/qpx.h>

static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd);

ti_query_t * ti_query_create(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_query_t * query = malloc(sizeof(ti_query_t));
    if (!query)
        return NULL;

    query->pkg_id = pkg->id;
    query->flags = 0;
    query->target = NULL;  /* root */
    query->parseres = NULL;
    query->raw = ti_raw_new(pkg->data, pkg->n);
    query->stream = ti_grab(stream);
    query->res_statements = NULL;

    if (!query->raw)
    {
        ti_query_destroy(query);
        return NULL;
    }

    return query;
}

void ti_query_destroy(ti_query_t * query)
{
    if (!query)
        return;
    if (query->parseres)
        cleri_parse_free(query->parseres);
    vec_destroy(query->res_statements, (vec_destroy_cb) ti_res_destroy);
    ti_stream_drop(query->stream);
    ti_db_drop(query->target);
    free(query->querystr);
    free(query->raw);
    free(query);
}

int ti_query_unpack(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    const char * ebad = "invalid query request, see "TI_DOCS"#query_request";
    qp_unpacker_t unpacker;
    qp_obj_t key, val;

    qp_unpacker_init2(&unpacker, query->raw->data, query->raw->n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, ebad);
        goto finish;
    }

    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "query"))
        {
            if (!qp_is_raw(qp_next(&unpacker, &val)))
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }
            query->querystr = qpx_obj_raw_to_str(&val);
            if (!query->querystr)
            {
                ex_set_alloc(e);
                goto finish;
            }
            continue;
        }

        if (qp_is_raw_equal_str(&key, "target"))
        {
            (void) qp_next(&unpacker, &val);
            query->target = ti_dbs_get_by_qp_obj(&val, e);
            ti_grab(query->target);
            if (e->nr)
                goto finish;
            /* query->target maybe NULL in case when the target is root */
            continue;
        }

        if (!qp_is_map(qp_next(&unpacker, NULL)))
        {
            ex_set(e, EX_BAD_DATA, ebad);
            goto finish;
        }
    }

finish:
    return e->nr;
}

int ti_query_parse(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    query->parseres = cleri_parse(ti()->langdef, query->querystr);
    if (!query->parseres)
    {
        ex_set_alloc(e);
        goto finish;
    }
    if (!query->parseres->is_valid)
    {
        int i = cleri_parse_strn(
                e->msg,
                EX_MAX_SZ,
                query->parseres,
                &langdef_translate);

        assert_log(i<EX_MAX_SZ, "expecting >= max size %d>=%d", i, EX_MAX_SZ);

        /* we will certainly not hit the max size, but just to be safe */
        e->n = i < EX_MAX_SZ ? i : EX_MAX_SZ - 1;
        e->nr = EX_QUERY_ERROR;
        goto finish;
    }
finish:
    return e->nr;
}

int ti_query_investigate(ti_query_t * query, ex_t * e)
{
    cleri_children_t * child;
    int nstatements = 0;

    assert (e->nr == 0);

    child = query->parseres->tree   /* root */
            ->children->node        /* sequence <comment, list> */
            ->children->next->node  /* list */
            ->children;             /* first child or NULL */

    while(child)
    {
        ++nstatements;
        query__investigate_recursive(query, child->node);  /* scope */

        if (!child->next)
            break;
        child = child->next->next;                  /* skip delimiter */
    }

    query->res_statements = vec_new(nstatements);
    if (!query->res_statements)
        ex_set_alloc(e);

    return e->nr;
}

void ti_query_run(ti_query_t * query)
{
    cleri_children_t * child;
    ex_t * e = ex_use();

    child = query->parseres->tree   /* root */
            ->children->node        /* sequence <comment, list> */
            ->children->next->node  /* list */
            ->children;             /* first child or NULL */

    if (query->target)
    {
        while (child)
        {
            assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

            ti_res_t * res = ti_res_create(query->target);
            if (!res)
            {
                ex_set_alloc(e);
                goto send;
            }

            VEC_push(query->res_statements, res);

            res_scope(res, child->node, e);
            if (e->nr)
                goto send;

            /* TODO: handle res->collect->n which means we first need to
             *       fetch things */
            assert_log(res->collect->n == 0, "collecting is not implemented");

            if (!child->next)
                break;
            child = child->next->next;                  /* skip delimiter */
        }
    }
    else
    {
        /* TODO: implement root */
        assert_log(0, "root queries not implemented yet");
    }

send:
    ti_query_send(query, e);
}

void ti_query_send(ti_query_t * query, ex_t * e)
{
    ti_pkg_t * pkg;
    qpx_packer_t * packer;
    if (e->nr)
        goto pkg_err;

    /* TODO: we can probably make an educated guess about the required size */
    packer = qpx_packer_create(65536, 8);
    if (!packer)
        goto alloc_err;

    (void) qp_add_array(&packer);

    /* we should have a result for each statement */
    assert (query->res_statements->n == query->res_statements->sz);

    for (vec_each(query->res_statements, ti_res_t, res))
    {
        assert (res);
        assert (res->val);
        if (ti_val_to_packer(res->val, &packer))
            goto alloc_err;
    }

    if (qp_close_array(packer))
        goto alloc_err;

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_RES_QUERY);
    pkg->id = query->pkg_id;

    goto finish;

alloc_err:
    if (packer)
        qp_packer_destroy(packer);
    ex_set_alloc(e);

pkg_err:
    pkg = ti_pkg_err(query->pkg_id, e);

finish:

    if (!pkg || ti_clients_write(query->stream, pkg))
    {
        free(pkg);
        log_critical(EX_ALLOC_S);
    }
    ti_query_destroy(query);

}

static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd)
{
    /*
     * TODO: this is not optimized in any way, but we will do this later since
     *       we might want to perform 'other' tasks at this point.
     */
    cleri_children_t * child;

    for (child = nd->children; child; child = child->next)
    {
        if (    langdef_nd_is_update_function(child->node) ||
                child->node == CLERI_GID_ASSIGNMENT)
            query->flags |= TI_QUERY_FLAG_WILL_UPDATE;

        if (child->node->children)
            query__investigate_recursive(query, child->node);
    }
}



