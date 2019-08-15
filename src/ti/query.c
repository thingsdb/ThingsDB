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
#include <ti/procedures.h>
#include <ti/epkg.h>
#include <ti/proto.h>
#include <ti/query.h>
#include <ti/nil.h>
#include <ti/task.h>
#include <util/qpx.h>
#include <util/strx.h>

#define QUERY_DOC_ TI_SEE_DOC("#query")
#define CALL_REQUEST_DOC_ TI_SEE_DOC("#call-request")

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
                log_critical(EX_MEMORY_S);
                break;
            }

            rpkg = ti_rpkg_create(pkg);
            if (!rpkg)
            {
                log_critical(EX_MEMORY_S);
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
        log_critical(EX_MEMORY_S);
        return;
    }

    /* send tasks to watchers if required */
    if (query->target)
        query__task_to_watchers(query);

    /* store event package in archive */
    if (ti_archive_push(epkg))
        log_critical(EX_MEMORY_S);

    ti_nodes_write_rpkg((ti_rpkg_t *) epkg);
    ti_epkg_drop(epkg);
}

static int query_unpack_blobs(
        ti_query_t * query,
        qp_unpacker_t * unp,
        size_t * max_raw,
        ex_t * e)
{
    qp_obj_t val;
    ssize_t n;
    qp_types_t tp = qp_next(unp, NULL);

    if (query->blobs)
    {
        ex_set(e, EX_BAD_DATA,
                "`blobs` found twice in the request"QUERY_DOC_);
        return e->nr;
    }

    if (!qp_is_array(tp))
    {
        ex_set(e, EX_BAD_DATA,
                "expecting `blobs` to be an array"QUERY_DOC_);
        return e->nr;
    }

    n = tp == QP_ARRAY_OPEN ? -1 : (ssize_t) tp - QP_ARRAY0;

    query->blobs = vec_new(n < 0 ? 8 : n);
    if (!query->blobs)
    {
        ex_set_mem(e);
        return e->nr;
    }

    while(n--)
    {
        ti_raw_t * blob;
        tp = qp_next(unp, &val);

        if (qp_is_close(tp))
            break;

        if (!qp_is_raw(tp))
        {
            ex_set(e, EX_BAD_DATA,
                    "expecting a `blobs` array to contain type `raw`"
                    QUERY_DOC_);
            return e->nr;
        }

        blob = ti_raw_create(val.via.raw, val.len);
        if (!blob || vec_push(&query->blobs, blob))
        {
            ex_set_mem(e);
            return e->nr;
        }

        if (blob->n > *max_raw)
            *max_raw = blob->n;
    }
    return 0;
}

static int query__node_or_thingsdb_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);

    qp_unpacker_t unpacker;
    qp_obj_t key, val;
    size_t max_raw = 0;

    query->syntax.pkg_id = pkg_id;
    query->root = ti()->thing0;

    qp_unpacker_init2(&unpacker, data, n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, "invalid request"QUERY_DOC_);
        goto finish;
    }

    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "query"))
        {
            if (query->querystr)
            {
                ex_set(e, EX_BAD_DATA,
                        "`query` found twice in the request"QUERY_DOC_);
                return e->nr;
            }

            if (!qp_is_raw(qp_next(&unpacker, &val)))
            {
                ex_set(e, EX_BAD_DATA,
                        "expecting `query` to be or type `raw`"QUERY_DOC_);
                return e->nr;
            }

            query->querystr = qpx_obj_raw_to_str(&val);
            if (!query->querystr)
            {
                ex_set_mem(e);
                goto finish;
            }

            continue;
        }

        if (qp_is_raw_equal_str(&key, "blobs"))
        {
            if (query_unpack_blobs(query, &unpacker, &max_raw, e))
                return e->nr;
            continue;
        }

        log_debug(
                "unexpected `query` key in map: `%.*s`",
                key.len, (const char *) key.via.raw);
    }

    if (!query->querystr)
        ex_set(e, EX_BAD_DATA, "missing `query` in request"QUERY_DOC_);

finish:
    return e->nr;
}

ti_query_t * ti_query_create(ti_stream_t * stream, ti_user_t * user)
{
    ti_query_t * query = malloc(sizeof(ti_query_t));
    if (!query)
        return NULL;

    query->rval = NULL;
    query->syntax.flags = 0;
    query->syntax.deep = 1;
    query->syntax.val_cache_n = 0;
    query->target = NULL;  /* node or thingsdb when NULL */
    query->parseres = NULL;
    query->closure = NULL;
    query->stream = ti_grab(stream);
    query->user = ti_grab(user);
    query->ev = NULL;
    query->blobs = NULL;
    query->querystr = NULL;
    query->val_cache = NULL;
    query->vars = vec_new(7);  /* with some initial size; we could find the
                                * exact number in the syntax (maybe), but then
                                * we also must allow closures to still grow
                                * this vector, so a fixed size may be the best
                                */
    ti_chain_init(&query->chain);

    if (!query->vars)
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

    vec_destroy(query->vars, (vec_destroy_cb) ti_prop_destroy);
    assert (!ti_chain_is_set(&query->chain));

    ti_val_drop((ti_val_t *) query->closure);
    vec_destroy(query->val_cache, (vec_destroy_cb) ti_val_drop);
    ti_stream_drop(query->stream);
    ti_user_drop(query->user);
    ti_collection_drop(query->target);
    ti_event_drop(query->ev);
    ti_val_drop(query->rval);
    vec_destroy(query->blobs, (vec_destroy_cb) ti_val_drop);

    free(query->querystr);
    free(query);
}

int ti_query_run_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    static ti_raw_t node = {
            tp: TI_VAL_RAW,
            n: 5,
            data: ".node"
    };
    static ti_raw_t thingsdb = {
            tp: TI_VAL_RAW,
            n: 9,
            data: ".thingsdb"
    };
    qp_unpacker_t unpacker;
    qp_obj_t qp_target, qp_procedure;
    ti_procedure_t * procedure;
    vec_t * procedures;
    ti_val_t * argval;
    size_t idx = 0;

    assert (e->nr == 0);
    assert (query->val_cache == NULL);

    query->syntax.flags |= TI_SYNTAX_FLAG_AS_PROCEDURE;
    query->syntax.pkg_id = pkg_id;

    qp_unpacker_init2(&unpacker, data, n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA,
                "expecting a `call` request to contain an array"
                CALL_REQUEST_DOC_);
        return e->nr;
    }

    (void) qp_next(&unpacker, &qp_target);
    if (!qp_is_raw(qp_next(&unpacker, &qp_procedure)))
    {
        ex_set(e, EX_BAD_DATA,
                "expecting the array to a call request to have a "
                "second value of type `raw`"CALL_REQUEST_DOC_);
        return e->nr;
    }

    if (qpx_obj_endswith_raw(&qp_target, &node))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot make a `call` request to the `node` scope"
                CALL_REQUEST_DOC_);
        return e->nr;
    }

    if (qpx_obj_endswith_raw(&qp_target, &thingsdb))
    {
        query->syntax.flags |= TI_SYNTAX_FLAG_THINGSDB;
        query->root = ti()->thing0;
        procedures = ti()->procedures;
    }
    else
    {
        query->target = ti_collections_get_by_qp_obj(&qp_target, e);
        if (e->nr)
            return e->nr;
        ti_incref(query->target);
        query->syntax.flags |= TI_SYNTAX_FLAG_THINGSDB;
        query->root = query->target->root;
        procedures = query->target->procedures;
    }

    procedure = ti_procedures_by_strn(
            procedures,
            (const char *) qp_procedure.via.raw,
            qp_procedure.len);

    if (!procedure)
    {
        ex_set(e, EX_INDEX_ERROR, "procedure `%.*s` not found",
                (int) qp_procedure.len,
                (char *) qp_procedure.via.raw);
        return e->nr;
    }

    if (procedure->closure->flags & TI_VFLAG_CLOSURE_WSE)
        query->syntax.flags |= TI_SYNTAX_FLAG_EVENT;

    query->val_cache = vec_new(procedure->closure->vars->n);
    if (!query->val_cache)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (vec_each(procedure->closure->vars, ti_prop_t, prop), ++idx)
    {
        /*
         * Make things explicit NULL, existing things should be parsed using
         * and integer and the t() function, and new things must be created
         * by the procedure and not given as argument.
         */
        argval = ti_val_from_unp(&unpacker, NULL);
        if (!argval)
        {
            ex_set(e, EX_INDEX_ERROR,
                "argument `%zu` for procedure `%.*s` is invalid (or missing)",
                idx,
                (int) qp_procedure.len,
                (char *) qp_procedure.via.raw);
            return e->nr;
        }
        VEC_push(query->val_cache, argval);
    }

    if (!qp_is_close(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_INDEX_ERROR,
            "too much arguments for procedure `%.*s`",
            (int) qp_procedure.len,
            (char *) qp_procedure.via.raw);
        return e->nr;
    }

    query->closure = procedure->closure;
    ti_incref(query->closure);

    return 0;
}

int ti_query_node_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    query->syntax.flags |= TI_SYNTAX_FLAG_NODE;
    return query__node_or_thingsdb_unpack(query, pkg_id, data, n, e);
}

int ti_query_thingsdb_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    query->syntax.flags |= TI_SYNTAX_FLAG_THINGSDB;
    return query__node_or_thingsdb_unpack(query, pkg_id, data, n, e);
}

int ti_query_collection_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);

    qp_unpacker_t unpacker;
    qp_obj_t key, val;
    size_t max_raw = 0;

    query->syntax.flags |= TI_SYNTAX_FLAG_COLLECTION;
    query->syntax.pkg_id = pkg_id;

    qp_unpacker_init2(&unpacker, data, n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, "invalid request"QUERY_DOC_);
        return e->nr;
    }

    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "query"))
        {
            if (query->querystr)
            {
                ex_set(e, EX_BAD_DATA,
                        "`query` found twice in the request"QUERY_DOC_);
                return e->nr;
            }

            if (!qp_is_raw(qp_next(&unpacker, &val)))
            {
                ex_set(e, EX_BAD_DATA,
                        "expecting `query` to be or type `raw`"QUERY_DOC_);
                return e->nr;
            }

            if (val.len > max_raw)
                max_raw = val.len;

            query->querystr = qpx_obj_raw_to_str(&val);
            if (!query->querystr)
            {
                ex_set_mem(e);
                return e->nr;
            }
            continue;
        }

        if (qp_is_raw_equal_str(&key, "collection"))
        {
            if (query->target)
            {
                ex_set(e, EX_BAD_DATA,
                        "`target` found twice in the request"QUERY_DOC_);
                return e->nr;
            }

            (void) qp_next(&unpacker, &val);
            query->target = ti_collections_get_by_qp_obj(&val, e);
            if (e->nr)
                return e->nr;
            ti_incref(query->target);
            continue;
        }

        if (qp_is_raw_equal_str(&key, "blobs"))
        {
            if (query_unpack_blobs(query, &unpacker, &max_raw, e))
                return e->nr;
            continue;
        }

        log_debug(
                "unexpected `query-collection` key in map: `%.*s`",
                key.len, (const char *) key.via.raw);
    }

    if (!query->querystr)
    {
        ex_set(e, EX_BAD_DATA, "missing `query` in request"QUERY_DOC_);
        return e->nr;
    }

    if (!query->target)
    {
        ex_set(e, EX_BAD_DATA, "missing `collection` in request"QUERY_DOC_);
        return e->nr;
    }

    query->root = query->target->root;

    if (max_raw >= query->target->quota->max_raw_size)
        ex_set(e, EX_MAX_QUOTA,
                "maximum raw size quota of %zu bytes is reached"
                TI_SEE_DOC("#quotas"), query->target->quota->max_raw_size);

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
        ex_set_mem(e);
        return e->nr;
    }

    if (!query->parseres->is_valid)
    {
        int i = cleri_parse_strn(e->msg, EX_MAX_SZ, query->parseres, NULL);

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
    cleri_children_t * seqchildren;
    assert (e->nr == 0);

    seqchildren = query->parseres->tree /* root */
            ->children->node            /* sequence <comment, list, [deep]> */
            ->children;

    /* list statements */
    ti_syntax_investigate(&query->syntax, seqchildren->next->node);

    /*
     * Create value cache for primitives. (if required)
     */
    if (    query->syntax.val_cache_n &&
            !(query->val_cache = vec_new(query->syntax.val_cache_n)))
        ex_set_mem(e);

    return e->nr;
}

void ti_query_run(ti_query_t * query)
{
    cleri_children_t * child, * seqchild;
    ex_t * e = ex_use();

    if (query->syntax.flags & TI_SYNTAX_FLAG_AS_PROCEDURE)
    {
        if (query->closure->flags & TI_VFLAG_CLOSURE_WSE)
        {
            assert (query->ev);
            query->syntax.flags |= TI_SYNTAX_FLAG_WSE;
        }
        (void) ti_closure_call(query->closure, query, query->val_cache, e);
        goto stop;
    }

    seqchild = query->parseres->tree    /* root */
        ->children->node                /* sequence <comment, list, [deep]> */
        ->children->next;               /* list */

    child = seqchild->node->children;   /* first child or NULL */

    if (!child)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        goto stop;
    }

    while (1)
    {
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_do_scope(query, child->node, e))
            break;

        if (!child->next || !(child = child->next->next))
            break;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    /* optional deep */
    ti_do_may_set_deep(&query->syntax.deep, seqchild->next, e);

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
    ex_set_mem(e);

pkg_err:
    ++ti()->counters->queries_with_error;
    pkg = ti_pkg_client_err(query->syntax.pkg_id, e);

finish:
    if (!pkg || ti_stream_write_pkg(query->stream, pkg))
    {
        free(pkg);
        log_critical(EX_MEMORY_S);
    }

    ti_query_destroy(query);
}

ti_prop_t * ti_query_var_get(ti_query_t * query, ti_name_t * name)
{
    for (vec_each_rev(query->vars, ti_prop_t, prop))
        if (prop->name == name)
            return prop;
    return NULL;
}
