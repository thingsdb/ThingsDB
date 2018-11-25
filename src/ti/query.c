/*
 * ti/query.c
 */
#include <assert.h>
#include <langdef/nd.h>
#include <langdef/translate.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/dbs.h>
#include <ti/proto.h>
#include <ti/query.h>
#include <ti/res.h>
#include <ti/arrow.h>
#include <ti/task.h>
#include <ti/root.h>
#include <util/qpx.h>

static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd);
static _Bool query__swap_opr(
        ti_query_t * query,
        cleri_children_t * parent,
        uint32_t parent_gid);
static void query__task_to_watchers(ti_query_t * query);
static void query__nd_cache_cleanup(cleri_node_t * node);
static void query__collect_destroy_cb(vec_t * names);

ti_query_t * ti_query_create(ti_stream_t * stream)
{
    ti_query_t * query = malloc(sizeof(ti_query_t));
    if (!query)
        return NULL;

    query->collect = omap_create();
    if (!query->collect)
    {
        free(query);
        return NULL;
    }

    query->flags = 0;
    query->target = NULL;  /* root */
    query->parseres = NULL;
    query->stream = ti_grab(stream);
    query->statements = NULL;
    query->ev = NULL;
    query->blobs = NULL;
    query->querystr = NULL;
    query->nd_cache_count = 0;
    query->nd_cache = NULL;

    return query;
}

void ti_query_destroy(ti_query_t * query)
{
    if (!query)
        return;
    /* must destroy nd_cache before clearing the parse result */
    vec_destroy(query->nd_cache, (vec_destroy_cb) query__nd_cache_cleanup);

    if (query->parseres)
        cleri_parse_free(query->parseres);

    vec_destroy(
            query->statements,
            query->target
                ? (vec_destroy_cb) ti_res_destroy
                : (vec_destroy_cb) ti_root_destroy);
    ti_stream_drop(query->stream);
    ti_db_drop(query->target);
    vec_destroy(query->blobs, (vec_destroy_cb) ti_raw_drop);
    free(query->querystr);
    omap_destroy(query->collect, (omap_destroy_cb) query__collect_destroy_cb);

    free(query);
}

int ti_query_unpack(ti_query_t * query, ti_pkg_t * pkg, ex_t * e)
{
    assert (e->nr == 0);
    const char * ebad = "invalid query request, see "TI_DOCS"#query";
    qp_unpacker_t unpacker;
    qp_obj_t key, val;

    query->pkg_id = pkg->id;

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

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

        if (qp_is_raw_equal_str(&key, "blobs"))
        {
            ssize_t n;
            qp_types_t tp = qp_next(&unpacker, NULL);

            if (!qp_is_array(tp))
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }

            n = tp == QP_ARRAY_OPEN ? -1 : (ssize_t) tp - QP_ARRAY0;

            query->blobs = vec_new(n < 0 ? 8 : n);
            if (!query->blobs)
            {
                ex_set_alloc(e);
                goto finish;
            }

            while(n-- && qp_is_raw(qp_next(&unpacker, &val)))
            {
                ti_raw_t * blob = ti_raw_create(val.via.raw, val.len);
                if (!blob || vec_push(&query->blobs, blob))
                {
                    ex_set_alloc(e);
                    goto finish;
                }
            }
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
    query->parseres = cleri_parse2(
            ti()->langdef,
            query->querystr,
            CLERI_FLAG_EXPECTING_DISABLED);  /* only error position */
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

    query->statements = vec_new(nstatements);
    if (!query->statements || (
            query->nd_cache_count && !(
                    query->nd_cache = vec_new(query->nd_cache_count)
            )
    ))
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

    log_debug(
        "\n  query     : \x1B[33m%s\x1B[0m"
        "\n  event     : %s"
        "\n  node cache: %lu",
        query->querystr,
        query->ev ? "true" : "false",
        query->nd_cache_count);

    if (query->target)
    {
        while (child)
        {
            assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

            ti_res_t * res = ti_res_create(
                    query->target,
                    query->ev,
                    query->blobs,
                    query->nd_cache,
                    query->collect);
            if (!res)
            {
                ex_set_alloc(e);
                goto done;
            }

            VEC_push(query->statements, res);

            ti_res_scope(res, child->node, e);
            if (e->nr)
                goto done;

            if (!child->next)
                break;
            child = child->next->next;                  /* skip delimiter */
        }
    }
    else
    {
        while (child)
        {
            assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

            ti_root_t * root = ti_root_create();
            if (!root)
            {
                ex_set_alloc(e);
                goto done;
            }

            root->ev = query->ev;  /* NULL if no update is required */

            VEC_push(query->statements, root);

            ti_root_scope(root, child->node, e);
            if (e->nr)
                goto done;

            if (!child->next)
                break;
            child = child->next->next;                  /* skip delimiter */
        }
    }

done:
    if (query->ev)
        query__task_to_watchers(query);

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
    assert (query->statements->n == query->statements->sz);

    /* ti_res_t or ti_root_t */
    for (vec_each(query->statements, ti_res_t, res))
    {
        assert (res);
        assert (res->rval);
        if (ti_val_to_packer(res->rval, &packer, 0))
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

    if (!pkg || ti_stream_write_pkg(query->stream, pkg))
    {
        free(pkg);
        log_critical(EX_ALLOC_S);
    }
    ti_query_destroy(query);

}

static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        switch (nd                              /* sequence */
                ->children->node                /* choice */
                ->children->node->cl_obj->gid)  /* keyword or name */
        {
        case CLERI_GID_F_ID:
        case CLERI_GID_F_RET:
            return;  /* arguments will be ignored */
        case CLERI_GID_F_DEL:
        case CLERI_GID_F_NEW:
        case CLERI_GID_F_PUSH:
        case CLERI_GID_F_RENAME:
        case CLERI_GID_F_SET:
        case CLERI_GID_F_UNSET:
            query->flags |= TI_QUERY_FLAG_EVENT;
        }
        /* skip to arguments */
        query__investigate_recursive(
                query,
                nd->children->next->next->node);
        return;
    case CLERI_GID_ASSIGNMENT:
        query->flags |= TI_QUERY_FLAG_EVENT;
        /* skip to scope */
        query__investigate_recursive(
                query,
                nd->children->next->next->node);
        return;
    case CLERI_GID_ARROW:
        {
            uint8_t flags = query->flags;
            query->flags = 0;
            query__investigate_recursive(
                    query,
                    nd->children->next->next->node);

            langdef_nd_flag(nd,
                    query->flags & TI_QUERY_FLAG_EVENT
                    ? TI_ARROW_FLAG|TI_ARROW_FLAG_WSE
                    : TI_ARROW_FLAG);

            query->flags |= flags;
        }
        return;
    case CLERI_GID_COMMENT:
    case CLERI_GID_INDEX:
        /* all with children we can skip */
        return;
    case CLERI_GID_PRIMITIVES:
        switch (nd->children->node->cl_obj->gid)
        {
            case CLERI_GID_T_STRING:
            case CLERI_GID_T_REGEX:
                ++query->nd_cache_count;
                nd->children->node->data = NULL;    /* init data to null */
        }
        return;
    case CLERI_GID_OPERATIONS:
        (void) query__swap_opr(query, nd->children->next, 0);
        return;

    }

    for (cleri_children_t * child = nd->children; child; child = child->next)
        query__investigate_recursive(query, child->node);
}

static _Bool query__swap_opr(
        ti_query_t * query,
        cleri_children_t * parent,
        uint32_t parent_gid)
{
    cleri_node_t * node;
    cleri_node_t * nd = parent->node;
    cleri_children_t * childb;
    uint32_t gid;

    assert( nd->cl_obj->tp == CLERI_TP_RULE ||
            nd->cl_obj->tp == CLERI_TP_PRIO ||
            nd->cl_obj->tp == CLERI_TP_THIS);

    node = nd->cl_obj->tp == CLERI_TP_PRIO ?
            nd->children->node :
            nd->children->node->children->node;

    if (node->cl_obj->gid == CLERI_GID_SCOPE)
    {
        query__investigate_recursive(query, node);
        return false;
    }

    assert (node->cl_obj->tp == CLERI_TP_SEQUENCE);

    gid = node->cl_obj->gid;
    childb = node->children->next->next;

    (void) query__swap_opr(query, node->children, gid);
    if (query__swap_opr(query, childb, gid))
    {
        cleri_node_t * tmp;
        cleri_children_t * bchilda;
        parent->node = childb->node;
        tmp = childb->node->cl_obj->tp == CLERI_TP_PRIO ?
                childb->node->children->node :
                childb->node->children->node->children->node;
        bchilda = tmp->children;
        gid = tmp->cl_obj->gid;
        tmp = bchilda->node;
        bchilda->node = nd;
        childb->node = tmp;
    }

    return gid > parent_gid;
}

static void query__task_to_watchers(ti_query_t * query)
{
    omap_iter_t iter = omap_iter(query->ev->tasks);
    for (omap_each(iter, ti_task_t, task))
    {
        if (ti_thing_has_watchers(task->thing))
        {
            ti_rpkg_t * rpkg;
            ti_pkg_t * pkg = ti_task_watch(task);
            if (!pkg)
            {
                log_critical(EX_ALLOC_S);
                break;
            }

            rpkg = ti_rpkg_create(pkg);
            if (!rpkg)
            {
                log_critical(EX_ALLOC_S);
                break;
            }

            for (vec_each(task->thing->watchers, ti_watch_t, watch))
            {
                if (ti_stream_is_closed(watch->stream))
                    continue;

                if (ti_stream_write_rpkg(watch->stream, rpkg))
                    log_critical(EX_INTERNAL_S);
            }

            ti_rpkg_drop(rpkg);
        }
    }
}

static void query__nd_cache_cleanup(cleri_node_t * node)
{
    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_STRING:
        ti_raw_drop(node->data);
        return;
    case CLERI_GID_T_REGEX:
        ti_regex_drop(node->data);
        return;
    }
}


static void query__collect_destroy_cb(vec_t * names)
{
    vec_destroy(names, (vec_destroy_cb) ti_name_drop);
}
