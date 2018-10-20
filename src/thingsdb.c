/*
 * thingsdb.c
 */
#include <assert.h>
#include <props.h>
#include <stdlib.h>
#include <thingsdb.h>
#include <ti/api.h>
#include <ti/db.h>
#include <ti/event.h>
#include <ti/misc.h>
#include <ti/signals.h>
#include <ti/store.h>
#include <ti/user.h>
#include <util/fx.h>
#include <util/strx.h>
#include <util/qpx.h>
#include <util/lock.h>

static const uint8_t thingsdb__def_redundancy = 3;
const char * thingsdb__fn = "thingsdb.qp";
const int thingsdb__fn_schema = 0;          /* thingsdb config */
const int thingsdb__fn_store_schema = 0;    /* store running db */

static uv_loop_t loop_;
static thingsdb_t thingsdb;

static qp_packer_t * thingsdb__pack(void);
static int thingsdb__unpack(qp_res_t * res);
static void thingsdb__close_handles(uv_handle_t * handle, void * arg);

int thingsdb_create(void)
{
    thingsdb.flags = 0;
    thingsdb.redundancy = thingsdb__def_redundancy;
    thingsdb.fn = NULL;
    thingsdb.node = NULL;
    thingsdb.lookup = NULL;
    thingsdb.args = ti_args_new();
    thingsdb.access = vec_new(0);
    thingsdb.maint = ti_maint_new();

    if (    !thingsdb_args_create() ||
            !thingsdb_cfg_create() ||
            !thingsdb_clients_create() ||
            !thingsdb_nodes_create() ||
            !thingsdb_props_create() ||
            !thingsdb_users_create() ||
            !thingsdb_dbs_create() ||
            !thingsdb_events_create() ||
            !thingsdb.args ||
            !thingsdb.cfg ||
            !thingsdb.access ||
            !thingsdb.maint)
    {
        thingsdb_destroy();
        return -1;
    }

    return 0;
}

void thingsdb_destroy(void)
{
    free(thingsdb.fn);
    free(thingsdb.maint);
    ti_lookup_destroy(thingsdb.lookup);
    thingsdb_args_destroy();
    thingsdb_cfg_destroy();
    thingsdb_clients_destroy();
    thingsdb_nodes_destroy();
    thingsdb_events_destroy();
    thingsdb_dbs_destroy();
    thingsdb_nodes_destroy();
    thingsdb_users_destroy();
    thingsdb_props_destroy();
    vec_destroy(thingsdb.access, free);
    memset(&thingsdb, 0, sizeof(thingsdb_t));
}

thingsdb_t * thingsdb_get(void)
{
    return &thingsdb;
}

void thingsdb_init_logger(void)
{
    int n;
    char lname[255];
    size_t len = strlen(thingsdb.args->log_level);

#ifdef NDEBUG
    /* force colors while debugging... */
    if (thingsdb.args->log_colorized)
#endif
    {
        Logger.flags |= LOGGER_FLAG_COLORED;
    }

    for (n = 0; n < LOGGER_NUM_LEVELS; n++)
    {
        strcpy(lname, LOGGER_LEVEL_NAMES[n]);
        strx_lower_case(lname);
        if (strlen(lname) == len && strcmp(thingsdb.args->log_level, lname) == 0)
        {
            logger_init(stdout, n);
            return;
        }
    }
    assert (0);
}

int thingsdb_init_fn(void)
{
    thingsdb.fn = strx_cat(thingsdb.cfg->store_path, thingsdb__fn);
    return (thingsdb.fn) ? 0 : -1;
}

int thingsdb_build(void)
{
    ex_t * e = ex_use();
    int rc = -1;
    qp_packer_t * packer = ti_misc_init_query();
    if (!packer)
        return -1;

    thingsdb.events->commit_event_id = 0;
    thingsdb.events->next_event_id = 1;
    thingsdb.next_thing_id = 1;

    thingsdb.node = thingsdb_nodes_create_node(thingsdb.nodes->addr);
    if (!thingsdb.node)
        goto stop;

    if (thingsdb_save())
        goto stop;

//    ti_event_raw(event, packer->buffer, packer->len, e);
//    if (e->nr)
//    {
//        log_critical(e->msg);
//        goto stop;
//    }
//
//    if (ti_event_run(event) != 2 || ti_store())
//        goto stop;

    rc = 0;

stop:
    if (rc)
    {
        fx_rmdir(thingsdb.cfg->store_path);
        mkdir(thingsdb.cfg->store_path, 0700); /* no error checking required */
        ti_node_drop(thingsdb.node);
        thingsdb.node = NULL;
        vec_pop(thingsdb.nodes);
    }
    qp_packer_destroy(packer);
    return rc;
}

int thingsdb_read(void)
{
    int rc;
    ssize_t n;
    unsigned char * data = fx_read(thingsdb.fn, &n);
    if (!data) return -1;

    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, data, (size_t) n);
    qp_res_t * res = qp_unpacker_res(&unpacker, &rc);
    free(data);
    if (rc)
    {
        log_critical(qp_strerror(rc));
        return -1;
    }
    rc = thingsdb__unpack(res);
    qp_res_destroy(res);
    if (rc)
    {
        log_critical("unpacking has failed (%s)", thingsdb.fn);
        goto stop;
    }
    thingsdb.lookup = ti_lookup_create(
            thingsdb.nodes->vec->n,
            thingsdb.redundancy,
            thingsdb.nodes);
    if (!thingsdb.lookup) return -1;

stop:
    return rc;
}

int thingsdb_run(void)
{
    uv_loop_init(&loop_);
    thingsdb.loop = &loop_;

    if (ti_events_init(thingsdb.events) ||
        ti_signals_init() ||
        thingsdb_nodes_listen())
    {
        ti_term(SIGTERM);
    }

    if (thingsdb.node)
    {
        if (thingsdb_clients_listen() || ti_maint_start(thingsdb.maint))
        {
            ti_term(SIGTERM);
        }
        thingsdb.node->status = TI_NODE_STAT_READY;
    }

    uv_run(&thingsdb.loop, UV_RUN_DEFAULT);

    uv_walk(&thingsdb.loop, thingsdb__close_handles, NULL);

    uv_run(&thingsdb.loop, UV_RUN_DEFAULT);

    uv_loop_close(&thingsdb.loop);

    return 0;
}

int thingsdb_save(void)
{
    int rc;
    qp_packer_t * packer = thingsdb__pack();
    if (!packer)
        return -1;

    rc = fx_write(thingsdb.fn, packer->buffer, packer->len);
    if (rc)
        log_error("failed to write file: '%s'", thingsdb.fn);

    qp_packer_destroy(packer);
    return rc;
}

int thingsdb_lock(void)
{
    lock_t rc = lock_lock(thingsdb.cfg->store_path, LOCK_FLAG_OVERWRITE);

    switch (rc)
    {
    case LOCK_IS_LOCKED_ERR:
    case LOCK_PROCESS_NAME_ERR:
    case LOCK_WRITE_ERR:
    case LOCK_READ_ERR:
    case LOCK_MEM_ALLOC_ERR:
        log_error("%s (%s)", lock_str(rc), thingsdb.cfg->store_path);
        return -1;
    case LOCK_NEW:
        log_info("%s (%s)", lock_str(rc), thingsdb.cfg->store_path);
        break;
    case LOCK_OVERWRITE:
        log_warning("%s (%s)", lock_str(rc), thingsdb.cfg->store_path);
        break;
    default:
        break;
    }
    thingsdb.flags |= THINGSDB_FLAG_LOCKED;
    return 0;
}

int thingsdb_unlock(void)
{
    if (thingsdb.flags & THINGSDB_FLAG_LOCKED)
    {
        lock_t rc = lock_unlock(thingsdb.cfg->store_path);
        if (rc != LOCK_REMOVED)
        {
            log_error(lock_str(rc));
            return -1;
        }
    }
    return 0;
}

int thingsdb_store(const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(64);
    if (!packer) return -1;

    if (qp_add_map(&packer) ||
        qp_add_raw_from_str(packer, "schema") ||
        qp_add_int64(packer, thingsdb__fn_store_schema) ||
        qp_add_raw_from_str(packer, "commit_event_id") ||
        qp_add_int64(packer, (int64_t) thingsdb.events->commit_event_id) ||
        qp_add_raw_from_str(packer, "next_thing_id") ||
        qp_add_int64(packer, (int64_t) thingsdb.next_thing_id) ||
        qp_close_map(packer)) goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc) log_error("failed to write file: '%s'", fn);
    qp_packer_destroy(packer);
    return rc;
}

int thingsdb_restore(const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    unsigned char * data = fx_read(fn, &n);
    if (!data) return -1;

    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, data, (size_t) n);
    qp_res_t * res = qp_unpacker_res(&unpacker, &rcode);
    free(data);

    if (rcode)
    {
        log_critical(qp_strerror(rcode));
        return -1;
    }

    qp_res_t * schema, * qpcommit_event_id, * qpnext_thing_id;

    if (res->tp != QP_RES_MAP ||
        !(schema = qpx_map_get(res->via.map, "schema")) ||
        !(qpcommit_event_id = qpx_map_get(res->via.map, "commit_event_id")) ||
        !(qpnext_thing_id = qpx_map_get(res->via.map, "next_thing_id")) ||
        schema->tp != QP_RES_INT64 ||
        schema->via.int64 != thingsdb__fn_store_schema ||
        qpcommit_event_id->tp != QP_RES_INT64 ||
        qpnext_thing_id->tp != QP_RES_INT64)
        goto stop;

    thingsdb.events->commit_event_id = (uint64_t) qpcommit_event_id->via.int64;
    thingsdb.events->next_event_id = thingsdb.events->commit_event_id + 1;
    thingsdb.next_thing_id = (uint64_t) qpnext_thing_id->via.int64;

    rc = 0;

stop:
    if (rc) log_critical("failed to restore from file: '%s'", fn);
    qp_res_destroy(res);
    return rc;
}

uint64_t thingsdb_next_thing_id(void)
{
    return thingsdb.next_thing_id++;
}

_Bool thingsdb_manages_id(uint64_t id)
{
    return ti_node_manages_id(thingsdb.node, thingsdb.lookup, id);
}

static qp_packer_t * thingsdb__pack(void)
{
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer || qp_add_map(&packer))
        goto failed;

    /* schema */
    if (    qp_add_raw_from_str(packer, "schema") ||
            qp_add_int64(packer, thingsdb__fn_schema))
        goto failed;

    /* redundancy */
    if (    qp_add_raw_from_str(packer, "redundancy") ||
            qp_add_int64(packer, (int64_t) thingsdb.redundancy))
        goto failed;

    /* node */
    if (    qp_add_raw_from_str(packer, "node") ||
            qp_add_int64(packer, (int64_t) thingsdb.node->id))
        goto failed;

    /* nodes */
    if (    qp_add_raw_from_str(packer, "nodes") ||
            thingsdb_nodes_to_packer(&packer))
        goto failed;

    if (qp_close_map(packer))
        goto failed;

    return packer;

failed:
    if (packer)
        qp_packer_destroy(packer);
    return NULL;
}

static int thingsdb__unpack(qp_res_t * res)
{
    uint8_t node_id;
    qp_res_t * schema, * qpredundancy, * qpnode, * qpnodes;

    if (    res->tp != QP_RES_MAP ||
            !(schema = qpx_map_get(res->via.map, "schema")) ||
            !(qpredundancy = qpx_map_get(res->via.map, "redundancy")) ||
            !(qpnode = qpx_map_get(res->via.map, "node")) ||
            !(qpnodes = qpx_map_get(res->via.map, "nodes")) ||
            schema->tp != QP_RES_INT64 ||
            schema->via.int64 != thingsdb__fn_schema ||
            qpredundancy->tp != QP_RES_INT64 ||
            qpnode->tp != QP_RES_INT64 ||
            qpnodes->tp != QP_RES_ARRAY ||
            qpredundancy->via.int64 < 1 ||
            qpredundancy->via.int64 > 64 ||
            qpnode->via.int64 < 0)
        goto failed;

    thingsdb.redundancy = (uint8_t) qpredundancy->via.int64;
    node_id = (uint8_t) qpnode->via.int64;

    if (thingsdb_nodes_from_qpres(qpnodes))
        goto failed;

    if (node_id >= thingsdb.nodes->vec->n)
        goto failed;

    thingsdb.node = thingsdb_nodes_node_by_id(node_id);
    if (!thingsdb.node)
        goto failed;

    assert (thingsdb.node->id == node_id);

    return 0;

failed:
    log_critical("unpacking has failed (%s)", thingsdb.fn);
    thingsdb.node = NULL;
    return -1;
}

static void thingsdb__close_handles(uv_handle_t * handle, void * arg)
{
    if (uv_is_closing(handle)) return;

    switch (handle->type)
    {
    case UV_ASYNC:
    case UV_SIGNAL:
        uv_close(handle, NULL);
        break;
    case UV_TCP:
        ti_stream_close((ti_stream_t *) handle->data);
        break;
    default:
        log_error("unexpected handle type: %d", handle->type);
    }
}
