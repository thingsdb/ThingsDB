/*
 * ti_.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/db.h>
#include <ti/event.h>
#include <ti/names.h>
#include <ti/signals.h>
#include <ti/store.h>
#include <ti/auth.h>
#include <ti/user.h>
#include <ti/users.h>
#include <ti/dbs.h>
#include <ti/access.h>
#include <ti/proto.h>
#include <ti/things.h>
#include <ti.h>
#include <util/fx.h>
#include <util/strx.h>
#include <util/qpx.h>
#include <util/lock.h>
#include <langdef/langdef.h>
#include <unistd.h>

ti_t ti_;

/* settings, nodes etc. */
const char * ti__fn = "ti_.qp";
const int ti__fn_schema = 0;

static uv_loop_t loop_;

static qp_packer_t * ti__pack(void);
static int ti__unpack(qp_res_t * res);
static void ti__close_handles(uv_handle_t * handle, void * arg);

int ti_create(void)
{
    ti_.stored_event_id = 0;
    ti_.flags = 0;
    ti_.redundancy = 0;     /* initially from --init --redundancy x     */
    ti_.fn = NULL;
    ti_.node = NULL;
    ti_.lookup = NULL;
    ti_.store = NULL;
    ti_.next_thing_id = NULL;
    ti_.access = vec_new(0);
    ti_.langdef = compile_langdef();
    ti_.thing0 = ti_thing_create(0, NULL);
    if (    clock_gettime(TI_CLOCK_MONOTONIC, &ti_.boottime) ||
            gethostname(ti_.hostname, TI_MAX_HOSTNAME_SZ) ||
            ti_counters_create() ||
            ti_away_create() ||
            ti_args_create() ||
            ti_cfg_create() ||
            ti_archive_create() ||
            ti_clients_create() ||
            ti_nodes_create() ||
            ti_names_create() ||
            ti_users_create() ||
            ti_dbs_create() ||
            ti_events_create() ||
            ti_connect_create() ||
            !ti_.access ||
            !ti_.langdef)
    {
        ti_destroy();
        return -1;
    }

    return 0;
}

void ti_destroy(void)
{
    free(ti_.fn);

    /* usually the signal handler will make the stop calls,
     * but not if ti_run() is never called */
    ti_events_stop();
    ti_connect_stop();
    ti_away_stop();

    ti_archive_destroy();
    ti_lookup_destroy();
    ti_args_destroy();
    ti_cfg_destroy();
    ti_clients_destroy();
    ti_nodes_destroy();
    ti_dbs_destroy();
    ti_users_destroy();
    ti_names_destroy();
    ti_store_destroy();
    ti_thing_drop(ti_.thing0);
    ti_counters_destroy();  /* very last since counters can be updated */
    vec_destroy(ti_.access, (vec_destroy_cb) ti_auth_destroy);
    if (ti_.langdef)
        cleri_grammar_free(ti_.langdef);
    memset(&ti_, 0, sizeof(ti_t));
}

void ti_init_logger(void)
{
    int n;
    char lname[255];
    size_t len = strlen(ti_.args->log_level);

#ifdef NDEBUG
    /* force colors while debugging... */
    if (ti_.args->log_colorized)
#endif
    {
        Logger.flags |= LOGGER_FLAG_COLORED;
    }

    for (n = 0; n < LOGGER_NUM_LEVELS; n++)
    {
        strcpy(lname, LOGGER_LEVEL_NAMES[n]);
        strx_lower_case(lname);
        if (strlen(lname) == len && strcmp(ti_.args->log_level, lname) == 0)
        {
            logger_init(stdout, n);
            return;
        }
    }
    assert (0);
}

int ti_init(void)
{
    ti_.fn = strx_cat(ti_.cfg->storage_path, ti__fn);
    return (ti_.fn) ? ti_store_create() : -1;
}

int ti_build(void)
{
    int rc = -1;
    ti_event_t * ev = ti_event_initial();
    if (!ev)
        return rc;

    ti_.redundancy = ti_.args->redundancy;

    ti_.node = ti_nodes_create_node(&ti_.nodes->addr);
    if (!ti_.node || ti_save())
        goto failed;

    ti_.node->cevid = 0;
    ti_.node->next_thing_id = 1;
    ti_.events->next_event_id = 1;

    ti_.events->cevid = &ti_.node->cevid;
    ti_.next_thing_id = &ti_.node->next_thing_id;

    if (ti_event_run(ev))
        goto failed;

    ti_.node->cevid = ev->id;

    if (ti_store_store())
        goto failed;

    rc = 0;
    goto done;

failed:
    (void) fx_rmdir(ti_.cfg->storage_path);
    (void) mkdir(ti_.cfg->storage_path, 0700);
    ti_node_drop(ti_.node);
    ti_.node = NULL;
    (void *) vec_pop(ti_.nodes->vec);

done:
    ti_event_drop(ev);
    return rc;
}

int ti_read(void)
{
    int rc;
    ssize_t n;
    uchar * data = fx_read(ti_.fn, &n);
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
    rc = ti__unpack(res);
    qp_res_destroy(res);
    if (rc)
    {
        log_critical("unpacking has failed (%s)", ti_.fn);
        goto stop;
    }

    if (ti_lookup_create(ti_.redundancy, ti_.nodes->vec))
        return -1;

stop:
    return rc;
}

int ti_run(void)
{
    int rc;
    if (uv_loop_init(&loop_))
        return -1;

    ti_.loop = &loop_;

    if (ti_signals_init())
        goto failed;

    if (ti_events_start())
        goto failed;

    if (ti_archive_load())
        goto failed;

    if (ti_away_start())
        goto failed;

    /* TODO: this is probably not right yet, not all things should start
     *       when only listening for joining a pool
     */
    if (ti_.node)
    {
        if (ti_clients_listen())
            goto failed;
        ti_.node->status = TI_NODE_STAT_READY;
    }

    if (ti_nodes_listen())
        goto failed;

    if (ti_connect_start())
        goto failed;

    rc = uv_run(ti_.loop, UV_RUN_DEFAULT);
    goto finish;

failed:
    rc = -1;
    uv_stop(ti_.loop);

finish:
    if (uv_loop_close(ti_.loop))
    {
        uv_walk(ti_.loop, ti__close_handles, NULL);
        (void) uv_run(ti_.loop, UV_RUN_DEFAULT);
        return -(uv_loop_close(ti_.loop) || rc);
    }
    return rc;
}

void ti_stop(void)
{
    ti_archive_to_disk();
    ti_away_stop();
    ti_connect_stop();
    ti_events_stop();

    uv_stop(ti()->loop);
}

int ti_save(void)
{
    int rc;
    qp_packer_t * packer = ti__pack();
    if (!packer)
        return -1;

    rc = fx_write(ti_.fn, packer->buffer, packer->len);
    if (rc)
        log_error("failed to write file: `%s`", ti_.fn);

    qp_packer_destroy(packer);
    return rc;
}

int ti_lock(void)
{
    lock_t rc = lock_lock(ti_.cfg->storage_path, LOCK_FLAG_OVERWRITE);

    switch (rc)
    {
    case LOCK_IS_LOCKED_ERR:
    case LOCK_PROCESS_NAME_ERR:
    case LOCK_WRITE_ERR:
    case LOCK_READ_ERR:
    case LOCK_MEM_ALLOC_ERR:
        log_error("%s (%s)", lock_str(rc), ti_.cfg->storage_path);
        return -1;
    case LOCK_NEW:
        log_debug("%s (%s)", lock_str(rc), ti_.cfg->storage_path);
        break;
    case LOCK_OVERWRITE:
        log_warning("%s (%s)", lock_str(rc), ti_.cfg->storage_path);
        break;
    default:
        break;
    }
    ti_.flags |= TI_FLAG_LOCKED;
    return 0;
}

int ti_unlock(void)
{
    if (ti_.flags & TI_FLAG_LOCKED)
    {
        lock_t rc = lock_unlock(ti_.cfg->storage_path);
        if (rc != LOCK_REMOVED)
        {
            log_error(lock_str(rc));
            return -1;
        }
    }
    return 0;
}

ti_rpkg_t * ti_status_rpkg(void)
{
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    qpx_packer_t * packer = qpx_packer_create(29, 1);

    if (!packer)
        return NULL;

    (void) qp_add_array(&packer);
    (void) qp_add_int64(packer, *ti()->next_thing_id);
    (void) qp_add_int64(packer, *ti()->events->cevid);
    (void) qp_add_int64(packer, ti()->node->status);
    (void) qp_close_array(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_STATUS);
    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
    {
        free(pkg);
        return NULL;
    }
    return rpkg;
}

void ti_set_and_send_node_status(ti_node_status_t status)
{
    ti_rpkg_t * rpkg;

    log_debug("changing status of node `%s` from %s to %s",
            ti_node_name(ti()->node),
            ti_node_status_str(ti()->node->status),
            ti_node_status_str(status));

    ti()->node->status = status;

    rpkg = ti_status_rpkg();
    if (!rpkg)
    {
        log_critical(EX_ALLOC_S);
        return;
    }

    ti_nodes_write_rpkg(rpkg);
    ti_rpkg_drop(rpkg);
}

static qp_packer_t * ti__pack(void)
{
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer || qp_add_map(&packer))
        goto failed;

    /* schema */
    if (    qp_add_raw_from_str(packer, "schema") ||
            qp_add_int64(packer, ti__fn_schema))
        goto failed;

    /* redundancy */
    if (    qp_add_raw_from_str(packer, "redundancy") ||
            qp_add_int64(packer, (int64_t) ti_.redundancy))
        goto failed;

    /* node */
    if (    qp_add_raw_from_str(packer, "node") ||
            qp_add_int64(packer, (int64_t) ti_.node->id))
        goto failed;

    /* nodes */
    if (    qp_add_raw_from_str(packer, "nodes") ||
            ti_nodes_to_packer(&packer))
        goto failed;

    if (qp_close_map(packer))
        goto failed;

    return packer;

failed:
    if (packer)
        qp_packer_destroy(packer);
    return NULL;
}

static int ti__unpack(qp_res_t * res)
{
    uint8_t node_id;
    qp_res_t * schema, * qpredundancy, * qpnode, * qpnodes;

    if (    res->tp != QP_RES_MAP ||
            !(schema = qpx_map_get(res->via.map, "schema")) ||
            !(qpredundancy = qpx_map_get(res->via.map, "redundancy")) ||
            !(qpnode = qpx_map_get(res->via.map, "node")) ||
            !(qpnodes = qpx_map_get(res->via.map, "nodes")) ||
            schema->tp != QP_RES_INT64 ||
            schema->via.int64 != ti__fn_schema ||
            qpredundancy->tp != QP_RES_INT64 ||
            qpnode->tp != QP_RES_INT64 ||
            qpnodes->tp != QP_RES_ARRAY ||
            qpredundancy->via.int64 < 1 ||
            qpredundancy->via.int64 > 64 ||
            qpnode->via.int64 < 0)
        goto failed;

    ti_.redundancy = (uint8_t) qpredundancy->via.int64;
    node_id = (uint8_t) qpnode->via.int64;

    if (ti_nodes_from_qpres(qpnodes))
        goto failed;

    if (node_id >= ti_.nodes->vec->n)
        goto failed;

    ti_.node = ti_nodes_node_by_id(node_id);
    if (!ti_.node)
        goto failed;

    assert (ti_.node->id == node_id);

    return 0;

failed:
    log_critical("unpacking has failed (%s)", ti_.fn);
    ti_.node = NULL;
    return -1;
}


static void ti__close_handles(uv_handle_t * handle, void * UNUSED(arg))
{
    if (uv_is_closing(handle))
        return;

    switch (handle->type)
    {
    case UV_ASYNC:
    case UV_SIGNAL:
        uv_close(handle, NULL);
        break;
    case UV_TCP:
    case UV_NAMED_PIPE:
        if (handle->data)
            ti_stream_close((ti_stream_t *) handle->data);
        else
            uv_close(handle, NULL);
        break;
    case UV_TIMER:
        LOGC("non closing timer found");
        assert(0);
        break;
    default:
        log_error("unexpected handle type: %d", handle->type);
    }
}
