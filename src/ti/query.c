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
#include <ti/names.h>
#include <ti/query.h>
#include <ti/nil.h>
#include <ti/task.h>
#include <ti/data.h>
#include <util/qpx.h>
#include <util/strx.h>

/*
 *  tasks are ordered for low to high thing ids
 *   { [0, 0]: {0: [ {'job':...} ] } }
 */
static ti_epkg_t * query__epkg_event(ti_query_t * query)
{
    ti_epkg_t * epkg;
    ti_pkg_t * pkg;
    qpx_packer_t * packer;
    size_t sz = 24;
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
    /* store `no tasks` as scope 0, this will save space and a lookup */
    (void) qp_add_int(packer, tasks->n && query->collection
            ? query->collection->root->id
            : 0);
    (void) qp_close_array(packer);

    (void) qp_add_map(&packer);

    /* reset iterator */
    for (vec_each(tasks, ti_task_t, task))
    {
        (void) qp_add_int(packer, task->thing->id);
        (void) qp_add_array(&packer);
        for (vec_each(task->jobs, ti_data_t, data))
            (void) qp_add_qp(packer, data->data, data->n);
        (void) qp_close_array(packer);
    }
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);


    assert(packer->nest_sz == 3);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_EVENT);

    assert(pkg->n <= sz);

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
            if (!pkg || !(rpkg = ti_rpkg_create(pkg)))
            {
                ++ti()->counters->watcher_failed;
                log_critical(EX_MEMORY_S);
                free(pkg);
                break;
            }

            for (vec_each(task->thing->watchers, ti_watch_t, watch))
            {
                if (ti_stream_is_closed(watch->stream))
                    continue;

                if (ti_stream_write_rpkg(watch->stream, rpkg))
                {
                    ++ti()->counters->watcher_failed;
                    log_error(EX_INTERNAL_S);
                }
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
    if (query->collection)
        query__task_to_watchers(query);

    /* store event package in archive */
    if (ti_archive_push(epkg))
        log_critical(EX_MEMORY_S);

    ti_nodes_write_rpkg((ti_rpkg_t *) epkg);
    ti_epkg_drop(epkg);
}

static int query__set_scope(ti_query_t * query, ti_scope_t * scope, ex_t * e)
{
    switch (scope->tp)
    {
    case TI_SCOPE_COLLECTION_NAME:
        query->syntax.flags |= TI_SYNTAX_FLAG_COLLECTION;
        query->collection = ti_collections_get_by_strn(
                scope->via.collection_name.name,
                scope->via.collection_name.sz);

        if (query->collection)
            ti_incref(query->collection);
        else
            ex_set(e, EX_LOOKUP_ERROR, "collection `%.*s` not found",
                (int) scope->via.collection_name.sz,
                scope->via.collection_name.name);
        return e->nr;
    case TI_SCOPE_COLLECTION_ID:
        query->syntax.flags |= TI_SYNTAX_FLAG_COLLECTION;
        query->collection = ti_collections_get_by_id(scope->via.collection_id);
        if (query->collection)
            ti_incref(query->collection);
        else
            ex_set(e, EX_LOOKUP_ERROR, TI_COLLECTION_ID" not found",
                    scope->via.collection_id);
        return e->nr;
    case TI_SCOPE_NODE:
        query->syntax.flags |= TI_SYNTAX_FLAG_NODE;
        return e->nr;
    case TI_SCOPE_THINGSDB:
        query->syntax.flags |= TI_SYNTAX_FLAG_THINGSDB;
        return e->nr;
    }

    assert (0);
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
    query->collection = NULL;  /* node or thingsdb when NULL */
    query->parseres = NULL;
    query->closure = NULL;
    query->stream = ti_grab(stream);
    query->user = ti_grab(user);
    query->ev = NULL;
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
    ti_val_drop((ti_val_t *) query->closure);
    vec_destroy(query->val_cache, (vec_destroy_cb) ti_val_drop);
    ti_stream_drop(query->stream);
    ti_user_drop(query->user);
    ti_collection_drop(query->collection);
    ti_event_drop(query->ev);
    ti_val_drop(query->rval);

    free(query->querystr);
    free(query);
}

static int query__args(ti_query_t * query, ti_val_unp_t * vup, ex_t * e)
{
    size_t i, n;
    mp_obj_t obj, mp_key;
    ti_name_t * name;
    ti_prop_t * prop;
    ti_val_t * argval;

    if (mp_next(vup->up, &obj) != MP_MAP)
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting the array in a `query` request to have an "
                "optional third value of type `map`"DOC_QUERY);
        return e->nr;
    }

    for (i = 0, n = obj.via.sz; i < n; ++i)
    {
        if (mp_next(vup->up, &mp_key) != MP_STR ||
            !ti_name_is_valid_strn(mp_key.via.str.data, mp_key.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "each argument name in a `query` request "
                    "must follow the naming rules"DOC_NAMES);
            return e->nr;
        }

        name = ti_names_get(mp_key.via.str.data, mp_key.via.str.n);
        if (!name)
        {
            ex_set_mem(e);
            return e->nr;
        }

        argval = ti_val_from_unp_e(vup, e);
        if (!argval)
        {
            assert (e->nr);
            ex_append(e, " (in argument `%s`)", name->str);
            ti_name_drop(name);
            return e->nr;
        }

        prop = ti_prop_create(name, argval);
        if (!prop)
        {
            ti_name_drop(name);
            ti_val_drop(argval);
            ex_set_mem(e);
            return e->nr;
        }

        if (vec_push(&query->vars, prop))
        {
            ti_prop_destroy(prop);
            ex_set_mem(e);
            return e->nr;
        }
    }

    return 0;
}

int ti_query_unpack(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    mp_obj_t obj, mp_query;
    mp_unp_t up;
    mp_unp_init(&up, data, n);

    ti_val_unp_t vup = {
            .isclient = true,
            .collection = query->collection,
            .up = &up,
    };

    query->syntax.pkg_id = pkg_id;

    mp_next(&up, &obj);     /* array, TODO: make sure size 2 or 3 is checked */
    mp_skip(&up);           /* scope */

    if (mp_next(&up, &mp_query) != MP_STR)
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting the array in a `query` request to have a "
                "second value of type `"TI_VAL_STR_S"`"DOC_QUERY);
        return e->nr;
    }

    if (query__set_scope(query, scope, e))
        return e->nr;

    query->querystr = mp_strdup(&mp_query);
    if (!query->querystr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    return obj.via.sz == 2 ? 0 : query__args(query, &vup, e);
}

int ti_query_unp_run(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e)
{
    vec_t * procedures = NULL;
    mp_obj_t obj, mp_procedure;
    ti_procedure_t * procedure;
    ti_val_t * argval;
    size_t nargs, idx = 0;

    mp_unp_t up;
    mp_unp_init(&up, data, n);

    ti_val_unp_t vup = {
            .isclient = true,
            .collection = NULL,
            .up = &up,
    };

    assert (e->nr == 0);
    assert (query->val_cache == NULL);

    query->syntax.flags |= TI_SYNTAX_FLAG_AS_PROCEDURE;
    query->syntax.pkg_id = pkg_id;

    mp_next(&up, &obj);     /* array, TODO: make sure size is at least 2 */
    mp_skip(&up);           /* scope */

    nargs = obj.via.sz - 2;

    if (mp_next(&up, &mp_procedure) != MP_STR)
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting the array in a `run` request to have a "
                "second value of type `"TI_VAL_STR_S"`"DOC_PROCEDURES_API);
        return e->nr;
    }

    switch (scope->tp)
    {
    case TI_SCOPE_NODE:
        ex_set(e, EX_BAD_DATA,
                "cannot make a `run` request to the `node` scope"
                DOC_PROCEDURES_API);
        return e->nr;
    case TI_SCOPE_THINGSDB:
        query->syntax.flags |= TI_SYNTAX_FLAG_THINGSDB;
        procedures = ti()->procedures;
        vup.collection = NULL;
        break;
    case TI_SCOPE_COLLECTION_NAME:
    case TI_SCOPE_COLLECTION_ID:
        if (query__set_scope(query, scope, e))
            return e->nr;
        procedures = query->collection->procedures;
        vup.collection = query->collection;
        break;
    }

    procedure = ti_procedures_by_strn(
            procedures,
            mp_procedure.via.str.data,
            mp_procedure.via.str.n);

    if (!procedure)
    {
        ex_set(e, EX_LOOKUP_ERROR, "procedure `%.*s` not found",
                (int) mp_procedure.via.str.n,
                mp_procedure.via.str.data);
        return e->nr;
    }

    if (nargs != procedure->closure->vars->n)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
            "procedure `%.*s` takes %zu argument%s but %d %s given%s",
            (int) mp_procedure.via.str.n,
            mp_procedure.via.str.data,
            procedure->closure->vars->n,
            procedure->closure->vars->n == 1 ? "" : "s",
            nargs,
            nargs==1 ? "was" : "were");
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
        argval = ti_val_from_unp_e(&vup, e);
        if (!argval)
        {
            assert (e->nr);
            ex_append(e, " (argument %zu for procedure `%.*s`)",
                idx,
                (int) mp_procedure.via.str.n,
                mp_procedure.via.str.data);
            return e->nr;
        }
        VEC_push(query->val_cache, argval);
    }

    query->closure = procedure->closure;
    ti_incref(query->closure);

    return 0;
}

int ti_query_parse(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    query->parseres = cleri_parse2(
            ti()->langdef,
            query->querystr,
            TI_CLERI_PARSE_FLAGS);

    if (!query->parseres)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (!query->parseres->is_valid)
    {
        cleri_parse_free(query->parseres);

        query->parseres = cleri_parse2(ti()->langdef, query->querystr, 0);

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
    cleri_children_t * seqchildren;
    assert (e->nr == 0);

    seqchildren = query->parseres->tree /* root */
            ->children->node            /* sequence <comment, list> */
            ->children;

    /* list statements */
    ti_syntax_probe(&query->syntax, seqchildren->next->node);

    /*
     * Create value cache for immutable, names and things.
     */
    if (    query->syntax.val_cache_n &&
            !(query->val_cache = vec_new(query->syntax.val_cache_n)))
        ex_set_mem(e);

    return e->nr;
}

void ti_query_run(ti_query_t * query)
{
    cleri_children_t * child, * seqchild;
    ex_t e = {0};

    if (query->syntax.flags & TI_SYNTAX_FLAG_AS_PROCEDURE)
    {
        if (query->closure->flags & TI_VFLAG_CLOSURE_WSE)
        {
            assert (query->ev);
            query->syntax.flags |= TI_SYNTAX_FLAG_WSE;
        }
        (void) ti_closure_call(query->closure, query, query->val_cache, &e);
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
        assert (child->node->cl_obj->gid == CLERI_GID_STATEMENT);

        if (ti_do_statement(query, child->node, &e))
            break;

        if (!child->next || !(child = child->next->next))
            break;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    if (e.nr == EX_RETURN)
        e.nr = 0;

stop:
    if (query->ev)
        query__event_handle(query);  /* errors will be logged only */

    ti_query_send(query, &e);
}

void ti_query_send(ti_query_t * query, ex_t * e)
{
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    msgpack_sbuffer_init(&buffer);

    ti_val_may_change_pack_sz(query->rval, &buffer.alloc);
    buffer.size = 8;

    if (e->nr)
        goto pkg_err;

    msgpack_packer_init(&pk, f, msgpack_sbuffer_write);

    packer = qpx_packer_create(alloc_sz, nest_sz);
    if (!packer)
        goto alloc_err;

    if (ti_val_to_pk(query->rval, &pk, (int) query->syntax.deep))
    {
        /* TODO: create node config help, size in config etc.
            ex_set(e, EX_TOO_LARGE,
                    "result too large; "
                    "modify your query or increase the maximum result size");
        */
        goto alloc_err;
    }

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

ti_thing_t * ti_query_thing_from_id(
        ti_query_t * query,
        int64_t thing_id,
        ex_t * e)
{
    ti_thing_t * thing;

    if (!query->collection)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "scope `%s` has no stored things; "
                "you might want to query a `@collection` scope?",
                ti_query_scope_name(query));
        return NULL;
    }

    thing = thing_id
            ? ti_collection_thing_by_id(query->collection, (uint64_t) thing_id)
            : NULL;

    if (!thing)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "collection `%.*s` has no `thing` with id %"PRId64,
                (int) query->collection->name->n,
                (char *) query->collection->name->data,
                thing_id);
        return NULL;
    }

    ti_incref(thing);
    return thing;
}
