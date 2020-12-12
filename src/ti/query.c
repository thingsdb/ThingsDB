
/*
 * ti/query.c
 */
#include <assert.h>
#include <doc.h>
#include <errno.h>
#include <langdef/nd.h>
#include <langdef/translate.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/api.h>
#include <ti/closure.h>
#include <ti/collections.h>
#include <ti/collection.inline.h>
#include <ti/data.h>
#include <ti/do.h>
#include <ti/epkg.h>
#include <ti/epkg.inline.h>
#include <ti/event.h>
#include <ti/gc.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/procedures.h>
#include <ti/proto.h>
#include <ti/qcache.h>
#include <ti/query.h>
#include <ti/query.inline.h>
#include <ti/task.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <ti/wrap.h>
#include <util/strx.h>


/*
 *  tasks are ordered for low to high thing ids
 *   { [0, 0]: {0: [ {'job':...} ] } }
 */
static ti_epkg_t * query__epkg_event(ti_query_t * query)
{
    size_t init_buffer_sz = 40;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_epkg_t * epkg;
    ti_pkg_t * pkg;
    vec_t * tasks = query->ev->_tasks;

    for (vec_each(tasks, ti_task_t, task))
        init_buffer_sz += task->approx_sz;

    if (mp_sbuffer_alloc_init(&buffer, init_buffer_sz, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    /* key */
    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, query->ev->id);
    msgpack_pack_uint64(&pk, tasks->n && query->collection
            ? query->collection->root->id
            : 0);

    /* value */
    msgpack_pack_map(&pk, tasks->n);
    for (vec_each(tasks, ti_task_t, task))
    {
        msgpack_pack_uint64(&pk, task->thing->id);
        msgpack_pack_array(&pk, task->jobs->n);
        for (vec_each(task->jobs, ti_data_t, data))
            mp_pack_append(&pk, data->data, data->n);
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_EVENT, buffer.size);

    assert(pkg->n <= init_buffer_sz);

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
                ++ti.counters->watcher_failed;
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
                    ++ti.counters->watcher_failed;
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

int ti_query_apply_scope(ti_query_t * query, ti_scope_t * scope, ex_t * e)
{
    switch (scope->tp)
    {
    case TI_SCOPE_COLLECTION_NAME:
        query->qbind.flags |= TI_QBIND_FLAG_COLLECTION;
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
        query->qbind.flags |= TI_QBIND_FLAG_COLLECTION;
        query->collection = ti_collections_get_by_id(scope->via.collection_id);
        if (query->collection)
            ti_incref(query->collection);
        else
            ex_set(e, EX_LOOKUP_ERROR, TI_COLLECTION_ID" not found",
                    scope->via.collection_id);
        return e->nr;
    case TI_SCOPE_NODE:
        query->qbind.flags |= TI_QBIND_FLAG_NODE;
        return e->nr;
    case TI_SCOPE_THINGSDB:
        query->qbind.flags |= TI_QBIND_FLAG_THINGSDB;
        return e->nr;
    }

    assert (0);
    return e->nr;
}

ti_query_t * ti_query_create(uint8_t flags)
{
    ti_query_t * query = calloc(1, sizeof(ti_query_t));
    if (!query)
        return NULL;

    query->qbind.deep = 1;
    query->flags = flags;
    query->vars = vec_new(7);  /* with some initial size; we could find the
                                * exact number in the syntax (maybe), but then
                                * we also must allow closures to still grow
                                * this vector, so a fixed size may be the best
                                */
    if (!query->vars)
    {
        free(query);
        return NULL;
    }
    return query;
}

ti_query_t * ti_query_create_strn(const char * str, size_t n, uint8_t flags)
{
    ti_query_t * query = ti_query_create(flags);
    if (!query)
        return NULL;

    query->querystr = strndup(str, n);
    if (!query)
    {
        ti_query_destroy(query);
        return NULL;
    }

    return query;
}

void ti_query_init(ti_query_t * query, void * via, ti_user_t * user)
{
    if (query->flags & TI_QUERY_FLAG_API)
        query->via.api_request = ti_api_acquire((ti_api_request_t *) via);
    else
        query->via.stream = ti_grab((ti_stream_t *) via);

    query->user = ti_grab(user);
}

void ti_query_destroy(ti_query_t * query)
{
    if (!query)
        return;

    if (query->parseres)
        cleri_parse_free(query->parseres);

    vec_destroy(query->immutable_cache, (vec_destroy_cb) ti_val_unsafe_drop);

    if (query->flags & TI_QUERY_FLAG_API)
        ti_api_release(query->via.api_request);
    else
        ti_stream_drop(query->via.stream);

    ti_user_drop(query->user);
    ti_event_drop(query->ev);
    ti_val_drop(query->rval);

    if (query->vars)
    {
        while(query->vars->n)
            ti_prop_destroy(VEC_pop(query->vars));
        free(query->vars);
    }

    ti_val_drop((ti_val_t *) query->closure);

    ti_collection_drop(query->collection);

    /*
     * Garbage collection at least after cleaning the return value, otherwise
     * the value might already be destroyed.
     */
    ti_thing_clean_gc();

    free(query->querystr);
    free(query);
}

int ti_query_unpack_args(ti_query_t * query, mp_unp_t * up, ex_t * e)
{
    size_t i, n;
    mp_obj_t obj, mp_key;
    ti_name_t * name;
    ti_prop_t * prop;
    ti_val_t * argval;
    ti_vup_t vup = {
            .isclient = true,
            .collection = query->collection,
            .up = up,
    };

    if (mp_next(vup.up, &obj) == MP_END)
        return e->nr;  /* no arguments */

    if (obj.tp != MP_MAP)
    {
        if (obj.tp == MP_END)

        ex_set(e, EX_TYPE_ERROR,
                "expecting variable for a `query` to be of type `map`"
                DOC_SOCKET_QUERY);
        return e->nr;
    }

    for (i = 0, n = obj.via.sz; i < n; ++i)
    {
        if (mp_next(vup.up, &mp_key) != MP_STR ||
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

        argval = ti_val_from_vup_e(&vup, e);
        if (!argval)
        {
            assert (e->nr);
            ex_append(e, " (in argument `%s`)", name->str);
            ti_name_unsafe_drop(name);
            return e->nr;
        }

        prop = ti_prop_create(name, argval);
        if (!prop)
        {
            ti_name_unsafe_drop(name);
            ti_val_unsafe_drop(argval);
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
        const unsigned char * data,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    mp_obj_t obj, mp_query;
    mp_unp_t up;
    mp_unp_init(&up, data, n);

    query->pkg_id = pkg_id;

    mp_next(&up, &obj);     /* array with at least size 1 */
    mp_skip(&up);           /* scope */

    if (obj.via.sz < 2 || mp_next(&up, &mp_query) != MP_STR)
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting the code in a `query` request to be of type `string`"
                DOC_SOCKET_QUERY);
        return e->nr;
    }

    if (ti_query_apply_scope(query, scope, e))
        return e->nr;

    query->querystr = mp_strdup(&mp_query);
    if (!query->querystr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    return obj.via.sz == 2 ? 0 : ti_query_unpack_args(query, &up, e);
}

static int query__run_arr_props(
        ti_query_t * query,
        ti_vup_t * vup,
        ti_procedure_t * procedure,
        size_t nargs,
        ex_t * e)
{
    size_t n = procedure->closure->vars->n;
    n = nargs < n ? nargs : n;

    for (size_t idx = 0; idx < n; ++idx)
    {
        ti_val_t * val = ti_val_from_vup_e(vup, e);
        if (!val)
        {
            assert (e->nr);
            ex_append(e, " (argument %zu for procedure `%s`)",
                idx,
                procedure->name);
            return e->nr;
        }
        ti_val_unsafe_drop(vec_set(query->immutable_cache, val, idx));
    }
    return e->nr;
}

static int query__run_map_props(
        ti_query_t * query,
        ti_vup_t * vup,
        ti_procedure_t * procedure,
        size_t nargs,
        ex_t * e)
{
    mp_obj_t arg_name;
    ti_val_t * val;
    vec_t * vars = procedure->closure->vars;

    while (nargs--)
    {
        size_t idx = 0;
        if (mp_next(vup->up, &arg_name) != MP_STR)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "arguments keys must be of type `"TI_VAL_STR_S"` "
                    "but got type `%s` instead", mp_type_str(arg_name.tp));
            return e->nr;
        }

        for (vec_each(vars, ti_prop_t, prop), ++idx)
            if (mp_strn_eq(&arg_name, prop->name->str, prop->name->n))
                break;

        if (idx == vars->n)
        {
            mp_skip(vup->up);
            continue;
        }

        val = ti_val_from_vup_e(vup, e);
        if (!val)
        {
            assert (e->nr);
            ex_append(e, " (argument `%.*s` for procedure `%s`)",
                (int) arg_name.via.str.n,
                arg_name.via.str.data,
                procedure->name);
            return e->nr;
        }

        ti_val_unsafe_drop(vec_set(query->immutable_cache, val, idx));
    }
    return e->nr;
}

int ti_query_unp_run(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const unsigned char * data,
        size_t n,
        ex_t * e)
{
    smap_t * procedures = NULL;
    mp_obj_t obj, mp_procedure;
    ti_procedure_t * procedure;

    mp_unp_t up;
    mp_unp_init(&up, data, n);

    ti_vup_t vup = {
            .isclient = true,
            .collection = NULL,
            .up = &up,
    };

    assert (e->nr == 0);
    assert (query->immutable_cache == NULL);

    query->flags |= TI_QUERY_FLAG_AS_PROCEDURE;
    query->pkg_id = pkg_id;

    mp_next(&up, &obj);     /* array with at least size 1 */
    mp_skip(&up);           /* scope */


    if (obj.via.sz < 2 || mp_next(&up, &mp_procedure) != MP_STR)
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
        query->qbind.flags |= TI_QBIND_FLAG_THINGSDB;
        procedures = ti.procedures;
        vup.collection = NULL;
        break;
    case TI_SCOPE_COLLECTION_NAME:
    case TI_SCOPE_COLLECTION_ID:
        if (ti_query_apply_scope(query, scope, e))
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

    if (procedure->closure->flags & TI_VFLAG_CLOSURE_WSE)
        query->qbind.flags |= TI_QBIND_FLAG_EVENT;

    query->closure = procedure->closure;
    ti_incref(query->closure);

    query->immutable_cache = vec_new(procedure->closure->vars->n);
    if (!query->immutable_cache)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (vec_each(procedure->closure->vars, ti_prop_t, _))
        VEC_push(query->immutable_cache, ti_nil_get());

    mp_next(&up, &obj);

    switch (obj.tp)
    {
    case MP_ARR:
        return query__run_arr_props(query, &vup, procedure, obj.via.sz, e);
    case MP_MAP:
        return query__run_map_props(query, &vup, procedure, obj.via.sz, e);
    default:
        break;
    }
    return 0;
}

static inline int ti_query_investigate(ti_query_t * query, ex_t * e)
{
    cleri_children_t * seqchildren;
    assert (e->nr == 0);

    seqchildren = query->parseres->tree /* root */
            ->children->node            /* sequence <comment, list> */
            ->children;

    /* list statements */
    ti_qbind_probe(&query->qbind, seqchildren->next->node);

    /*
     * Create value cache for immutable, names and things.
     */
    if (    query->qbind.val_cache_n &&
            !(query->immutable_cache = vec_new(query->qbind.val_cache_n)))
        ex_set_mem(e);

    return e->nr;
}

int ti_query_parse(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    if (query->parseres)
        return e->nr;  /* already parsed and investigated */

    query->parseres = cleri_parse2(
            ti.langdef,
            query->querystr,
            TI_CLERI_PARSE_FLAGS);

    if (!query->parseres)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "query has reached the maximum recursion depth of %d",
                MAX_RECURSION_DEPTH);
        return e->nr;
    }

    if (!query->parseres->is_valid)
    {
        int i;
        cleri_parse_free(query->parseres);

        query->parseres = cleri_parse2(ti.langdef, query->querystr, 0);
        if (!query->parseres)
        {
            ex_set_mem(e);
            return e->nr;
        }

        i = cleri_parse_strn(
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

    return ti_query_investigate(query, e);
}

static void query__duration_log(
        ti_query_t * query,
        double duration,
        int log_level)
{
    if (query->flags & TI_QUERY_FLAG_AS_PROCEDURE)
    {
        log_with_level(log_level, "procedure took %f seconds to process: `%s`",
                duration,
                query->closure->node->str);
        return;
    }
    log_with_level(log_level, "query took %f seconds to process: `%s`",
            duration,
            query->querystr);
}

void ti_query_run(ti_query_t * query)
{
    cleri_children_t * child, * seqchild;
    ex_t e = {0};

    clock_gettime(TI_CLOCK_MONOTONIC, &query->time);

    if (query->flags & TI_QUERY_FLAG_AS_PROCEDURE)
    {
        if (query->closure->flags & TI_VFLAG_CLOSURE_WSE)
        {
            assert (query->ev);
            query->flags |= TI_QUERY_FLAG_WSE;
        }
        (void) ti_closure_call(query->closure, query, query->immutable_cache, &e);
        goto stop;
    }

#ifndef NDEBUG
    log_debug("[DEBUG] run query: %s", query->querystr);
#endif

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

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if (e.nr == EX_RETURN)
        e.nr = 0;

stop:
    if (query->ev)
        query__event_handle(query);  /* errors will be logged only */

    ti_query_send_response(query, &e);
}

static inline int query__pack_response(
        ti_query_t * query,
        msgpack_sbuffer * buffer,
        ex_t * e)
{
    msgpack_packer pk;
    msgpack_packer_init(&pk, buffer, msgpack_sbuffer_write);

    if (ti_val_to_pk(query->rval, &pk, (int) query->qbind.deep))
    {
        if (buffer->size > ti.cfg->result_size_limit)
            ex_set(e, EX_RESULT_TOO_LARGE,
                    "too much data to return; "
                    "try to use a lower `deep` value and/or `wrap` things to "
                    "reduce the data size"DOC_THING_WRAP);
        else
            ex_set_mem(e);

        msgpack_sbuffer_destroy(buffer);
        return e->nr;
    }

    if (buffer->size > ti.counters->largest_result_size)
        ti.counters->largest_result_size = buffer->size;

    return 0;
}

static int query__response_api(ti_query_t * query, ex_t * e)
{
    ti_api_request_t * ar = query->via.api_request;
    msgpack_sbuffer buffer;
    size_t alloc = 24;

    if (e->nr)
        goto response_err;

    ti_val_may_change_pack_sz(query->rval, &alloc);

    if (mp_sbuffer_alloc_init(&buffer, alloc, 0))
    {
        ex_set_mem(e);
        goto response_err;
    }

    if (query__pack_response(query, &buffer, e))
        goto response_err;

    return ti_api_close_with_response(ar, buffer.data, buffer.size);

response_err:
    return -(ti_api_close_with_err(ar, e) || 1);
}

static int query__response_pkg(ti_query_t * query, ex_t * e)
{
    ti_pkg_t * pkg;
    msgpack_sbuffer buffer;
    size_t alloc = 24;

    if (e->nr)
        goto pkg_err;

    ti_val_may_change_pack_sz(query->rval, &alloc);

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_pkg_t)))
    {
        ex_set_mem(e);
        goto pkg_err;
    }

    if (query__pack_response(query, &buffer, e))
        goto pkg_err;

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg,
            query->pkg_id,
            TI_PROTO_CLIENT_RES_DATA ,
            buffer.size);

    if (ti_stream_write_pkg(query->via.stream, pkg))
    {
        free(pkg);
        log_critical(EX_MEMORY_S);
    }
    return 0;

pkg_err:
    pkg = ti_pkg_client_err(query->pkg_id, e);
    if (!pkg || ti_stream_write_pkg(query->via.stream, pkg))
    {
        free(pkg);
        log_critical(EX_MEMORY_S);
    }
    return -1;
}

typedef int (*query__cb)(ti_query_t *, ex_t *);

void ti_query_send_response(ti_query_t * query, ex_t * e)
{
    double duration, warn = ti.cfg->query_duration_warn;
    query__cb cb = query->flags & TI_QUERY_FLAG_API
            ? query__response_api
            : query__response_pkg;

    if (cb(query, e))
    {
        log_debug("query failed: `%s`, %s: `%s`",
                query->flags & TI_QUERY_FLAG_AS_PROCEDURE
                ? query->closure->node->str
                : query->querystr,
                ex_str(e->nr),
                e->msg);

        ++ti.counters->queries_with_error;
        goto done;
    }

    duration = ti_counters_upd_success_query(&query->time);
    if (warn && duration > warn)
    {
        double derror = ti.cfg->query_duration_error;
        query__duration_log(
                query,
                duration,
                derror && duration > derror ? LOGGER_ERROR : LOGGER_WARNING
        );
    }

done:
    if (query->flags & TI_QUERY_FLAG_CACHE)
        ti_qcache_return(query);
    else
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

    /* No need to check for garbage collected things */
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

typedef struct
{
    ssize_t n;
    uint16_t type_id;
} query__count_t;

static inline int query__count(ti_thing_t * t, query__count_t * c)
{
    c->n += t->type_id == c->type_id;
    return 0;
}

ssize_t ti_query_count_type(ti_query_t * query, ti_type_t * type)
{
    query__count_t c = {
            .n = 0,
            .type_id = type->type_id
    };

    if (ti_type_is_wrap_only(type))
        return 0;

    if (ti_query_vars_walk(query->vars, (imap_cb) query__count, &c))
        return -1;

    (void) imap_walk(query->collection->things, (imap_cb) query__count, &c);
    (void) ti_gc_walk(query->collection->gc, (queue_cb) query__count, &c);

    return c.n;
}

static int query__var_walk_thing(ti_thing_t * thing, imap_t * imap);

static int query__get_things(ti_val_t * val, imap_t * imap)
{
    int rc;

    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_MP:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_ERROR:
    case TI_VAL_NAME:
    case TI_VAL_REGEX:
        break;
    case TI_VAL_THING:
        return query__var_walk_thing((ti_thing_t *) val, imap);
    case TI_VAL_WRAP:
        return query__var_walk_thing(((ti_wrap_t *) val)->thing, imap);
    case TI_VAL_ARR:
        if (ti_varr_may_have_things((ti_varr_t *) val))
            for (vec_each(VARR(val), ti_val_t, v))
                if ((rc = query__get_things(v, imap)))
                    return rc;
        break;
    case TI_VAL_SET:
        return imap_walk(VSET(val), (imap_cb) query__var_walk_thing, imap);
    case TI_VAL_CLOSURE:
    case TI_VAL_MEMBER:  /* things as a member have an id */
    case TI_VAL_TEMPLATE:
        break;
    }

    return 0;
}

static int query__var_walk_thing(ti_thing_t * thing, imap_t * imap)
{
    if (thing->id)
        return 0;

    switch (imap_add(imap, ti_thing_key(thing), thing))
    {
    case IMAP_ERR_ALLOC:
        return -1;
    case IMAP_ERR_EXIST:
        return 0;
    }

    /* SUCCESS, get a reference */

    ti_incref(thing);

    if (ti_thing_is_object(thing))
    {
        for (vec_each(thing->items, ti_prop_t, prop))
            if (query__get_things(prop->val, imap))
                return -1;
    }
    else
    {
        for (vec_each(thing->items, ti_val_t, val))
            if (query__get_things(val, imap))
                return -1;
    }
    return 0;
}

/*
 * While walking the things, each thing has a reference so it can not be
 * removed even if the callback tries to drop the thing.
 *
 * The callback will only be called on things without an ID.
 */
int ti_query_vars_walk(vec_t * vars, imap_cb cb, void * args)
{
    int rc;
    imap_t * imap = imap_create();
    if (!imap)
        return -1;

    for (vec_each(vars, ti_prop_t, prop))
        if ((rc = query__get_things(prop->val, imap)))
            goto fail;

    rc = imap_walk(imap, cb, args);

fail:
    imap_destroy(imap, (imap_destroy_cb) ti_val_unsafe_drop);
    return rc;
}

