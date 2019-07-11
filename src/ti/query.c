/*
 * ti/query.c
 */
#include <assert.h>
#include <errno.h>
#include <langdef/nd.h>
#include <langdef/translate.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti/closure.h>
#include <ti.h>
#include <ti/collections.h>
#include <ti/do.h>
#include <ti/epkg.h>
#include <ti/proto.h>
#include <ti/query.h>
#include <ti/nil.h>
#include <ti/task.h>
#include <util/qpx.h>
#include <util/strx.h>

/* maximum value we allow for the `deep` argument */
#define QUERY__MAX_DEEP 0x7f

/*
 *  tasks are ordered for low to high thing ids
 *   { [0, 0]: {0: [ {'job':...} ] } }
 */
static ti_epkg_t * query__epkg_event(ti_query_t * query)
{
    ti_epkg_t * epkg;
    ti_pkg_t * pkg;
    qpx_packer_t * packer;
    size_t sz = 0;
    vec_t * tasks = query->ev->_tasks;

    for (vec_each(tasks, ti_task_t, task))
        sz += task->approx_sz;

    /* nest size 3 is sufficient since jobs are already raw */
    packer = qpx_packer_create(sz, 3);
    if (!packer)
        return NULL;

    (void) qp_add_map(&packer);

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, query->ev->id);
    /* store `no tasks` as target 0, this will save space and a lookup */
    (void) qp_add_int(packer, tasks->n ? query->root->id : 0);
    (void) qp_close_array(packer);

    (void) qp_add_map(&packer);

    /* reset iterator */
    for (vec_each(tasks, ti_task_t, task))
    {
        (void) qp_add_int(packer, task->thing->id);
        (void) qp_add_array(&packer);
        for (vec_each(task->jobs, ti_raw_t, raw))
        {
            (void) qp_add_qp(packer, raw->data, raw->n);
        }
        (void) qp_close_array(packer);
    }
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_EVENT);
    epkg = ti_epkg_create(pkg, query->ev->id);
    if (!epkg)
    {
        free(pkg);
        return NULL;
    }
    return epkg;
}

/*
 * Void function, although errors can happen. Each error is critical since this
 * results in subscribers inconsistency.
 */
static void query__task_to_watchers(ti_query_t * query)
{
    vec_t * tasks = query->ev->_tasks;
    for (vec_each(tasks, ti_task_t, task))
    {
        if (ti_thing_has_watchers(task->thing))
        {
            ti_rpkg_t * rpkg;
            ti_pkg_t * pkg = ti_task_pkg_watch(task);
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

/*
 * Void function, although errors can happen. Each error is critical since this
 * results in nodes and subscribers inconsistency.
 */
static void query__event_handle(ti_query_t * query)
{
    ti_epkg_t * epkg = query__epkg_event(query);
    if (!epkg)
    {
        log_critical(EX_ALLOC_S);
        return;
    }

    /* send tasks to watchers if required */
    if (query->target)
        query__task_to_watchers(query);

    /* store event package in archive */
    if (ti_archive_push(epkg))
        log_critical(EX_ALLOC_S);

    ti_nodes_write_rpkg((ti_rpkg_t *) epkg);
    ti_epkg_drop(epkg);
}

static int query__node_db_unpack(
        const char * ebad,
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);

    qp_unpacker_t unpacker;
    qp_obj_t key, val;

    query->syntax.pkg_id = pkg_id;

    qp_unpacker_init2(&unpacker, data, n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, ebad);
        goto finish;
    }

    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "query"))
        {
            if (query->querystr || !qp_is_raw(qp_next(&unpacker, &val)))
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

        log_debug(
                "unexpected `query` key in map: `%.*s`",
                key.len, (const char *) key.via.raw);
    }

    if (!query->querystr)
        ex_set(e, EX_BAD_DATA, ebad);

finish:
    return e->nr;
}

ti_query_t * ti_query_create(ti_stream_t * stream, ti_user_t * user)
{
    ti_query_t * query = malloc(sizeof(ti_query_t));
    if (!query)
        return NULL;

    query->scope = NULL;
    query->rval = NULL;
    query->syntax.deep = 1;
    query->syntax.flags = 0;
    query->syntax.nd_val_cache_n = 0;
    query->target = NULL;  /* node or thingsdb when NULL */
    query->parseres = NULL;
    query->stream = ti_grab(stream);
    query->user = ti_grab(user);
    query->ev = NULL;
    query->blobs = NULL;
    query->querystr = NULL;
    query->nd_val_cache = NULL;
    query->tmpvars = NULL;

    return query;
}

void ti_query_destroy(ti_query_t * query)
{
    if (!query)
        return;

    if (query->parseres)
        cleri_parse_free(query->parseres);

    vec_destroy(query->nd_val_cache, (vec_destroy_cb) ti_val_drop);
    vec_destroy(query->tmpvars, (vec_destroy_cb) ti_prop_destroy);
    ti_stream_drop(query->stream);
    ti_user_drop(query->user);
    ti_collection_drop(query->target);
    ti_event_drop(query->ev);
    ti_val_drop(query->rval);
    vec_destroy(query->blobs, (vec_destroy_cb) ti_val_drop);
    free(query->querystr);
    free(query);
}

int ti_query_node_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    const char * ebad =
            "invalid `query-node` request"TI_SEE_DOC("#query-node");
    query->syntax.flags |= TI_SYNTAX_FLAG_NODE;
    return query__node_db_unpack(ebad, query, pkg_id, data, n, e);
}

int ti_query_thingsdb_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    const char * ebad =
            "invalid `query-thingsdb` request"TI_SEE_DOC("#query-thingsdb");
    query->syntax.flags |= TI_SYNTAX_FLAG_THINGSDB;
    return query__node_db_unpack(ebad, query, pkg_id, data, n, e);
}

int ti_query_collection_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    const char * ebad =
            "invalid `query-collection` request"TI_SEE_DOC("#query-thingsdb");

    qp_unpacker_t unpacker;
    qp_obj_t key, val;
    size_t max_raw = 0;

    query->syntax.flags |= TI_SYNTAX_FLAG_COLLECTION;
    query->syntax.pkg_id = pkg_id;

    qp_unpacker_init2(&unpacker, data, n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, ebad);
        goto finish;
    }

    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "query"))
        {
            if (query->querystr || !qp_is_raw(qp_next(&unpacker, &val)))
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }

            if (val.len > max_raw)
                max_raw = val.len;

            query->querystr = qpx_obj_raw_to_str(&val);
            if (!query->querystr)
            {
                ex_set_alloc(e);
                goto finish;
            }
            continue;
        }

        if (qp_is_raw_equal_str(&key, "collection"))
        {
            if (query->target)
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }

            (void) qp_next(&unpacker, &val);
            query->target = ti_collections_get_by_qp_obj(&val, e);
            if (e->nr)
                goto finish;
            ti_incref(query->target);
            continue;
        }

        if (qp_is_raw_equal_str(&key, "deep"))
        {
            if (!qp_is_int(qp_next(&unpacker, &val)))
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }
            if (val.via.int64 < 0 || val.via.int64 > QUERY__MAX_DEEP)
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }

            query->syntax.deep = (uint8_t) val.via.int64;
            continue;
        }

        if (qp_is_raw_equal_str(&key, "blobs"))
        {
            ssize_t n;
            qp_types_t tp = qp_next(&unpacker, NULL);

            if (query->blobs || !qp_is_array(tp))
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

                if (blob->n > max_raw)
                    max_raw = blob->n;
            }
            continue;
        }

        log_debug(
                "unexpected `query-collection` key in map: `%.*s`",
                key.len, (const char *) key.via.raw);
    }

    if (!query->querystr || !query->target)
        ex_set(e, EX_BAD_DATA, ebad);

    if (query->target && max_raw >= query->target->quota->max_raw_size)
        ex_set(e, EX_MAX_QUOTA,
                "maximum raw size quota of %zu bytes is reached"
                TI_SEE_DOC("#quotas"), query->target->quota->max_raw_size);

finish:
    return e->nr;
}

int ti_query_parse(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    query->parseres = cleri_parse2(
            ti()->langdef,
            query->querystr,
            CLERI_FLAG_EXPECTING_DISABLED|
            CLERI_FLAG_EXCLUDE_OPTIONAL);  /* only error position */

    if (!query->parseres)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    if (!query->parseres->is_valid)
    {
        int i = cleri_parse_strn(
                e->msg,
                EX_MAX_SZ,
                query->parseres,
                &langdef_translate);

        assert_log(i<EX_MAX_SZ, "expecting >= max size %d>=%d", i, EX_MAX_SZ);

        e->msg[EX_MAX_SZ] = '\0';

        log_warning("invalid syntax: `%s` (%s)", query->querystr, e->msg);

        /* we will certainly will not hit the max size, but just to be safe */
        e->n = i < EX_MAX_SZ ? i : EX_MAX_SZ - 1;
        e->nr = EX_SYNTAX_ERROR;
    }

    return e->nr;
}

int ti_query_investigate(ti_query_t * query, ex_t * e)
{
    cleri_children_t * child;

    assert (e->nr == 0);

    child = query->parseres->tree   /* root */
            ->children->node        /* sequence <comment, list> */
            ->children->next->node  /* list */
            ->children;             /* first child or NULL */

    while(child)
    {
        ti_syntax_investigate(&query->syntax, child->node);  /* scope */

        if (!child->next)
            break;
        child = child->next->next;                  /* skip delimiter */
    }

    /*
     * Create value cache for primitives. (if required)
     */
    if (    query->syntax.nd_val_cache_n &&
            !(query->nd_val_cache = vec_new(query->syntax.nd_val_cache_n)))
        ex_set_alloc(e);

    return e->nr;
}

void ti_query_run(ti_query_t * query)
{
    cleri_children_t * child;
    ex_t * e = ex_use();

    child = query->parseres->tree   /* root */
        ->children->node            /* sequence <comment, list> */
        ->children->next->node      /* list */
        ->children;                 /* first child or NULL */

    if (!child)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        goto stop;
    }

    query->root = query->target ? query->target->root : ti()->thing0;

    while (1)
    {
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_do_scope(query, child->node, e))
            break;

        ti_scope_leave(&query->scope, NULL);

        if (!child->next || !(child = child->next->next))
            break;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

stop:
    if (query->ev)
        query__event_handle(query);  /* errors will be logged only */

    ti_query_send(query, e);
}

void ti_query_send(ti_query_t * query, ex_t * e)
{
    ti_pkg_t * pkg;
    qpx_packer_t * packer;
    if (e->nr)
        goto pkg_err;

    packer = qpx_packer_create(65536, query->syntax.deep + 1);
    if (!packer)
        goto alloc_err;

    if (ti_val_to_packer(query->rval, &packer, query->syntax.deep))
        goto alloc_err;

    ++ti()->counters->queries_success;
    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_RES_QUERY);
    pkg->id = query->syntax.pkg_id;

    goto finish;

alloc_err:
    if (packer)
        qp_packer_destroy(packer);
    ex_set_alloc(e);

pkg_err:
    ++ti()->counters->queries_with_error;
    pkg = ti_pkg_client_err(query->syntax.pkg_id, e);

finish:
    if (!pkg || ti_stream_write_pkg(query->stream, pkg))
    {
        free(pkg);
        log_critical(EX_ALLOC_S);
    }

    ti_query_destroy(query);
}

/*
 * Removes and returns the `query->rval` if set. The reference will be moved
 * so the value is returned with a reference. If `query->rval` is not set,
 * a value from the scope is returned with a new reference.
 */
ti_val_t * ti_query_val_pop(ti_query_t * query)
{
    if (query->rval)
    {
        ti_val_t * val = query->rval;
        query->rval = NULL;
        return val;
    }

    if (query->scope->val)
    {
        ti_incref(query->scope->val);
        return query->scope->val;
    }

    ti_incref(query->scope->thing);
    return (ti_val_t *) query->scope->thing;
}

ti_prop_t * ti_query_tmpprop_get(ti_query_t * query, ti_name_t * name)
{
    if (!query->tmpvars)
        return NULL;

    for (vec_each(query->tmpvars, ti_prop_t, prop))
        if (prop->name == name)
            return prop;
    return NULL;
}
