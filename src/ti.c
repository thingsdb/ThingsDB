/*
 * ti_.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/collection.h>
#include <ti/collections.h>
#include <ti/event.h>
#include <ti/names.h>
#include <ti/proto.h>
#include <ti/regex.h>
#include <ti/signals.h>
#include <ti/store.h>
#include <ti/things.h>
#include <ti/user.h>
#include <ti/sync.h>
#include <ti/users.h>
#include <ti/version.h>
#include <tiinc.h>
#include <unistd.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/lock.h>
#include <util/qpx.h>
#include <util/strx.h>
#include <util/util.h>

ti_t ti_;

/* settings, nodes etc. */
const char * ti__fn = "ti_.qp";
const char * ti__node_fn = "node_.dat";
static int shutdown_counter = 3;
static uv_timer_t * shutdown_timer = NULL;
static uv_loop_t loop_;

static int ti__unpack(qp_res_t * res);
static void ti__shutdown_free(uv_handle_t * UNUSED(timer));
static void ti__shutdown_stop(void);
static void ti__shutdown_cb(uv_timer_t * shutdown);
static void ti__close_handles(uv_handle_t * handle, void * arg);
static void ti__stop(void);

int ti_create(void)
{
    ti_.flags = 0;
    ti_.fn = NULL;
    ti_.node_fn = NULL;
    ti_.build = NULL;
    ti_.node = NULL;
    ti_.lookup = NULL;
    ti_.store = NULL;
    ti_.next_thing_id = NULL;
    ti_.access_node = vec_new(0);
    ti_.access_thingsdb = vec_new(0);
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
            ti_collections_create() ||
            ti_events_create() ||
            ti_connect_create() ||
            ti_sync_create() ||
            !ti_.access_node ||
            !ti_.access_thingsdb ||
            !ti_.langdef)
    {
        /* ti_stop() is never called */
        ti_destroy();
        return -1;
    }

    return 0;
}

void ti_destroy(void)
{
    free(ti_.fn);
    free(ti_.node_fn);

    ti__stop();

    ti_build_destroy();
    ti_archive_destroy();
    ti_lookup_destroy(ti_.lookup);
    ti_args_destroy();
    ti_cfg_destroy();
    ti_clients_destroy();
    ti_nodes_destroy();
    ti_collections_destroy();
    ti_users_destroy();
    ti_names_destroy();
    ti_store_destroy();
    ti_val_drop((ti_val_t *) ti_.thing0);
    ti_counters_destroy();  /* very last since counters can be updated */
    vec_destroy(ti_.access_node, (vec_destroy_cb) ti_auth_destroy);
    vec_destroy(ti_.access_thingsdb, (vec_destroy_cb) ti_auth_destroy);
    if (ti_.langdef)
        cleri_grammar_free(ti_.langdef);
    memset(&ti_, 0, sizeof(ti_t));

    ti_val_drop_common();
}

int ti_init_logger(void)
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
            return logger_init(stdout, n);
        }
    }
    assert (0);
    return -1;
}

int ti_init(void)
{
    ti_.fn = strx_cat(ti_.cfg->storage_path, ti__fn);
    ti_.node_fn = strx_cat(ti_.cfg->storage_path, ti__node_fn);
    return (ti_.fn && ti_.node_fn) ? ti_store_create() : -1;
}

int ti_build(void)
{
    int rc = -1;
    ti_event_t * ev = NULL;
    char salt[CRYPTX_SALT_SZ];
    char encrypted[CRYPTX_SZ];

    cryptx_gen_salt(salt);
    cryptx("ThingsDB", salt, encrypted);

    ti_.node = ti_nodes_new_node(
            ti_args_get_zone(),
            ti_.cfg->node_port,
            "0.0.0.0",
            encrypted);
    if (!ti_.node)
        goto failed;

    ti_.lookup = ti_lookup_create(ti_.nodes->vec->n);

   if (ti_write_node_id(&ti_.node->id) || ti_save())
       goto failed;

    ti_.node->cevid = 0;
    ti_.node->next_thing_id = 1;
    ti_.events->next_event_id = 1;

    ti_.events->cevid = &ti_.node->cevid;
    ti_.next_thing_id = &ti_.node->next_thing_id;

    ev = ti_event_initial();
    if (!ev)
        goto failed;

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
    (void) vec_pop(ti_.nodes->vec);

done:
    ti_event_drop(ev);
    return rc;
}

int ti_write_node_id(uint8_t * node_id)
{
    assert (ti_.node_fn);

    int rc = -1;
    FILE * f = fopen(ti_.node_fn, "w");
    if (!f)
        goto finish;

    rc = -(fwrite(node_id, sizeof(uint8_t), 1, f) != 1);
    rc = -(fclose(f) || rc);

finish:
    if (rc)
        log_critical("error writing node id to `%s`", ti_.node_fn);

    return rc;
}

int ti_read_node_id(uint8_t * node_id)
{
    assert (ti_.node_fn);

    int rc = -1;
    FILE * f = fopen(ti_.node_fn, "r");
    if (!f)
        goto finish;

    rc = -(fread(node_id, sizeof(uint8_t), 1, f) != 1);
    rc = -(fclose(f) || rc);

finish:
    if (rc)
        log_critical("error reading node id from `%s`", ti_.node_fn);

    return rc;
}

int ti_read(void)
{
    int rc;
    ssize_t n;
    uchar * data = fx_read(ti_.fn, &n);
    if (!data)
        return -1;

    rc = ti_unpack(data, n);

    free(data);
    return rc;
}

int ti_unpack(uchar * data, size_t n)
{
    int rc;
    qp_res_t * res;
    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, data, (size_t) n);

    res = qp_unpacker_res(&unpacker, &rc);
    if (rc)
    {
        log_critical(qp_strerror(rc));
        return -1;
    }

    rc = ti__unpack(res);

    qp_res_destroy(res);
    if (rc)
        log_critical("unpacking has failed (%s)", ti_.fn);

    return rc;
}

int ti_run(void)
{
    int rc, attempts;

    ti_names_inject_common();

    if (ti_val_init_common())
        return -1;

    if (uv_loop_init(&loop_))
        return -1;

    ti_.loop = &loop_;

    if (ti_signals_init())
        goto failed;

    if (ti_events_start())
        goto failed;

    if (ti_.node)
    {
        ti_.node->status = TI_NODE_STAT_SYNCHRONIZING;

        if (ti_archive_init())
            goto failed;

        if (ti_archive_load())
            goto failed;

        if (ti_away_start())
            goto failed;

        if (ti_clients_listen())
            goto failed;

        if (ti_connect_start())
            goto failed;

        if (ti()->nodes->vec->n == 1)
        {
            ti_.node->status = TI_NODE_STAT_READY;
        }
        else if (ti_sync_start())
            goto failed;

    }
    else
    {
        if (ti_build_create())
            goto failed;
    }

    if (ti_nodes_listen())
        goto failed;

    rc = uv_run(ti_.loop, UV_RUN_DEFAULT);
    goto finish;

failed:
    rc = -1;
    ti_stop();

finish:
    attempts = 3;
    while (attempts--)
    {
        rc = uv_loop_close(ti_.loop);
        if (!rc)
            break;
        uv_walk(ti_.loop, ti__close_handles, NULL);
        (void) uv_run(ti_.loop, UV_RUN_DEFAULT);
    }
    return rc;
}

void ti_shutdown(void)
{
    if (ti_.node)
        ti_set_and_broadcast_node_status(TI_NODE_STAT_SHUTTING_DOWN);

    assert (shutdown_timer == NULL);

    shutdown_timer = malloc(sizeof(uv_timer_t));
    if (!shutdown_timer)
        goto fail0;

    if (uv_timer_init(ti_.loop, shutdown_timer))
        goto fail0;

    if (uv_timer_start(shutdown_timer, ti__shutdown_cb, 1000, 1000))
        goto fail1;

    return;

fail1:
    uv_close((uv_handle_t *) shutdown_timer, (uv_close_cb) free);
fail0:
    free(shutdown_timer);
    shutdown_timer = NULL;
    ti_stop();
}

void ti_stop(void)
{
    if (ti_.node)
    {
        ti_set_and_broadcast_node_status(TI_NODE_STAT_OFFLINE);

        (void) ti_collections_gc();
        (void) ti_archive_to_disk();
        (void) ti_archive_write_nodes_scevid();
    }
    ti__stop();
    uv_stop(ti()->loop);
}

int ti_save(void)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create2(32 + ti_.nodes->vec->n * 224, 3);
    if (!packer)
        return -1;

    if (ti_to_packer(&packer))
        goto stop;

    rc = fx_write(ti_.fn, packer->buffer, packer->len);
    if (rc)
        log_error("failed to write file: `%s`", ti_.fn);

stop:
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

ti_rpkg_t * ti_node_status_rpkg(void)
{
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    qpx_packer_t * packer = qpx_packer_create(34, 1);
    ti_node_t * ti_node = ti()->node;

    if (!packer)
        return NULL;

    (void) ti_node_info_to_packer(ti_node, &packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_INFO);
    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
        free(pkg);
    return rpkg;
}

ti_rpkg_t * ti_client_status_rpkg(void)
{
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    qpx_packer_t * packer = qpx_packer_create(20, 1);

    if (!packer)
        return NULL;

    (void) qp_add_raw_from_str(packer, ti_node_status_str(ti_.node->status));

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_NODE_STATUS);
    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
        free(pkg);
    return rpkg;
}

void ti_set_and_broadcast_node_status(ti_node_status_t status)
{
    ti_rpkg_t * client_rpkg;

    if (ti()->node->status == status)
        return;  /* node status is not changed */

    log_info("changing status of node `%s` from %s to %s",
            ti_node_name(ti()->node),
            ti_node_status_str(ti()->node->status),
            ti_node_status_str(status));

    ti()->node->status = status;

    ti_broadcast_node_info();

    client_rpkg = ti_client_status_rpkg();
    if (!client_rpkg)
    {
        log_critical(EX_ALLOC_S);
        goto fail;
    }
    ti_clients_write_rpkg(client_rpkg);

fail:
    ti_rpkg_drop(client_rpkg);
}

void ti_set_and_broadcast_node_zone(uint8_t zone)
{
    log_debug("changing zone of node `%s` from %s to %s",
            ti_node_name(ti()->node),
            ti_node_status_str(ti()->node->zone),
            ti_node_status_str(zone));

    ti()->node->zone = zone;

    ti_broadcast_node_info();
}

void ti_broadcast_node_info(void)
{
    ti_rpkg_t * node_rpkg;

    node_rpkg = ti_node_status_rpkg();
    if (!node_rpkg)
    {
        log_critical(EX_ALLOC_S);
        goto fail;
    }

    ti_nodes_write_rpkg(node_rpkg);

fail:
    ti_rpkg_drop(node_rpkg);
}

int ti_node_to_packer(qp_packer_t ** packer)
{
    struct timespec timing;
    (void) clock_gettime(TI_CLOCK_MONOTONIC, &timing);

    double uptime = util_time_diff(&ti_.boottime, &timing);

    return (
        qp_add_map(packer) ||
        qp_add_raw_from_str(*packer, "node_id") ||
        qp_add_int(*packer, ti_.node->id) ||
        qp_add_raw_from_str(*packer, "version") ||
        qp_add_raw_from_str(*packer, TI_VERSION) ||
        qp_add_raw_from_str(*packer, "libqpack_version") ||
        qp_add_raw_from_str(*packer, qp_version()) ||
        qp_add_raw_from_str(*packer, "libcleri_version") ||
        qp_add_raw_from_str(*packer, cleri_version()) ||
        qp_add_raw_from_str(*packer, "libuv_version") ||
        qp_add_raw_from_str(*packer, uv_version_string()) ||
        qp_add_raw_from_str(*packer, "libpcre2_version") ||
        qp_add_raw_from_str(*packer, TI_PCRE2_VERSION) ||
        qp_add_raw_from_str(*packer, "status") ||
        qp_add_raw_from_str(*packer, ti_node_status_str(ti_.node->status)) ||
        qp_add_raw_from_str(*packer, "zone") ||
        qp_add_int(*packer, ti_.node->zone) ||
        qp_add_raw_from_str(*packer, "log_level") ||
        qp_add_raw_from_str(*packer, Logger.level_name) ||
        qp_add_raw_from_str(*packer, "hostname") ||
        qp_add_raw_from_str(*packer, ti_.hostname) ||
        qp_add_raw_from_str(*packer, "client_port") ||
        qp_add_int(*packer, ti_.cfg->client_port) ||
        qp_add_raw_from_str(*packer, "node_port") ||
        qp_add_int(*packer, ti_.cfg->node_port) ||
        qp_add_raw_from_str(*packer, "ip_support") ||
        qp_add_raw_from_str(
            *packer,
            ti_tcp_ip_support_str(ti_.cfg->ip_support)) ||
        qp_add_raw_from_str(*packer, "storage_path") ||
        qp_add_raw_from_str(*packer, ti_.cfg->storage_path) ||
        qp_add_raw_from_str(*packer, "uptime") ||
        qp_add_double(*packer, uptime) ||
        qp_add_raw_from_str(*packer, "events_in_queue") ||
        qp_add_int(*packer, ti_.events->queue->n) ||
        qp_add_raw_from_str(*packer, "archived_on_disk") ||
        qp_add_int(*packer, ti_.archive->archived_on_disk) ||
        qp_add_raw_from_str(*packer, "archived_in_memory") ||
        qp_add_int(*packer, ti_.archive->queue->n) ||
        qp_add_raw_from_str(*packer, "archive_files") ||
        qp_add_int(*packer, ti_.archive->archfiles->n) ||
        qp_add_raw_from_str(*packer, "local_stored_event_id") ||
        qp_add_int(*packer, ti_.node->sevid) ||
        qp_add_raw_from_str(*packer, "local_commited_event_id") ||
        qp_add_int(*packer, ti_.node->cevid) ||
        qp_add_raw_from_str(*packer, "glocal_stored_event_id") ||
        qp_add_int(*packer, ti_nodes_sevid()) ||
        qp_add_raw_from_str(*packer, "global_commited_event_id") ||
        qp_add_int(*packer, ti_nodes_cevid()) ||
        qp_add_raw_from_str(*packer, "db_stored_event_id") ||
        qp_add_int(*packer, ti_.store->last_stored_event_id) ||
        qp_add_raw_from_str(*packer, "next_event_id") ||
        qp_add_int(*packer, ti_.events->next_event_id) ||
        qp_add_raw_from_str(*packer, "next_thing_id") ||
        qp_add_int(*packer, *ti_.next_thing_id) ||
        qp_close_map(*packer)
    );
}

ti_val_t * ti_node_as_qpval(void)
{
    ti_raw_t * raw;
    qp_packer_t * packer = qp_packer_create2(1024, 1);
    if (!packer)
        return NULL;

    raw = ti_node_to_packer(&packer)
            ? NULL
            : ti_raw_from_packer(packer);

    qp_packer_destroy(packer);
    return (ti_val_t *) raw;
}

static int ti__unpack(qp_res_t * res)
{
    uint8_t node_id;
    qp_res_t * schema, * qpnodes;

    if (    res->tp != QP_RES_MAP ||
            !(schema = qpx_map_get(res->via.map, "schema")) ||
            !(qpnodes = qpx_map_get(res->via.map, "nodes")) ||
            schema->tp != QP_RES_INT64 ||
            schema->via.int64 != TI_FN_SCHEMA ||
            qpnodes->tp != QP_RES_ARRAY)
        goto failed;

    if (ti_read_node_id(&node_id))
        goto failed;

    if (ti_nodes_from_qpres(qpnodes))
        goto failed;

    ti_.lookup = ti_lookup_create(ti_.nodes->vec->n);
    if (!ti_.lookup)
        goto failed;

    if (node_id >= ti_.nodes->vec->n)
        goto failed;

    ti_.node = ti_nodes_node_by_id(node_id);
    if (!ti_.node)
        goto failed;

    if (ti_args_has_zone())
        ti_.node->zone = ti_args_get_zone();

    return 0;

failed:
    ti_.node = NULL;
    return -1;
}

static void ti__shutdown_free(uv_handle_t * UNUSED(timer))
{
    free(shutdown_timer);
    shutdown_timer = NULL;
}

static void ti__shutdown_stop(void)
{
    (void) uv_timer_stop(shutdown_timer);
    uv_close((uv_handle_t *) shutdown_timer, ti__shutdown_free);
}

static void ti__shutdown_cb(uv_timer_t * UNUSED(timer))
{
    if (--shutdown_counter)
    {
        log_info("going offline in %d second%s",
                shutdown_counter, shutdown_counter == 1 ? "" : "s");
        return;
    }

    ti__shutdown_stop();
    ti_stop();
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
        if (handle == (uv_handle_t *) shutdown_timer)
            ti__shutdown_stop();
        else
        {
            log_error("non closing timer found");
            uv_timer_stop((uv_timer_t *) handle);
            uv_close(handle, NULL);
        }
        break;
    default:
        log_error("unexpected handle type: %d", handle->type);
    }
}

static void ti__stop(void)
{
    ti_away_stop();
    ti_connect_stop();
    ti_events_stop();
    ti_sync_stop();
}
