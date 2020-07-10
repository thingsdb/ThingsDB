/*
 * ti_.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/api.h>
#include <ti/auth.h>
#include <ti/collection.h>
#include <ti/collections.h>
#include <ti/do.h>
#include <ti/event.h>
#include <ti/names.h>
#include <ti/procedure.h>
#include <ti/proto.h>
#include <ti/qbind.h>
#include <ti/regex.h>
#include <ti/signals.h>
#include <ti/store.h>
#include <ti/sync.h>
#include <ti/things.h>
#include <ti/user.h>
#include <ti/users.h>
#include <ti/val.inline.h>
#include <ti/verror.h>
#include <ti/version.h>
#include <ti/web.h>
#include <tiinc.h>
#include <unistd.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/lock.h>
#include <util/mpack.h>
#include <util/strx.h>
#include <util/util.h>
#include <yajl/yajl_version.h>

#ifndef NDEBUG
/* these imports are required for sanity checks only */
#include <ti/nil.h>
#include <ti/vbool.h>
#include <ti/vint.h>
#endif

ti_t ti;

/* settings, nodes etc. */
const char * ti__fn = "ti.mp";
const char * ti__node_fn = ".node";
static int shutdown_counter = 6;
static uv_timer_t * shutdown_timer = NULL;
static uv_timer_t * delayed_start_timer = NULL;
static uv_loop_t loop_;

static void ti__shutdown_free(uv_handle_t * UNUSED(timer));
static void ti__shutdown_stop(void);
static void ti__shutdown_cb(uv_timer_t * shutdown);
static void ti__close_handles(uv_handle_t * handle, void * arg);
static void ti__stop(void);

int ti_create(void)
{
    ti.last_event_id = 0;
    ti.flags = 0;
    ti.fn = NULL;
    ti.node_fn = NULL;
    ti.build = NULL;
    ti.node = NULL;
    ti.store = NULL;
    ti.access_node = vec_new(0);
    ti.access_thingsdb = vec_new(0);
    ti.procedures = vec_new(0);
    ti.langdef = compile_langdef();
    ti.thing0 = ti_thing_o_create(0, 0, NULL);
    if (    clock_gettime(TI_CLOCK_MONOTONIC, &ti.boottime) ||
            ti_counters_create() ||
            ti_away_create() ||
            ti_args_create() ||
            ti_cfg_create() ||
            ti_archive_create() ||
            ti_clients_create() ||
            ti_nodes_create() ||
            ti_backups_create() ||
            ti_names_create() ||
            ti_users_create() ||
            ti_collections_create() ||
            ti_events_create() ||
            ti_connect_create() ||
            ti_sync_create() ||
            !ti.access_node ||
            !ti.access_thingsdb ||
            !ti.procedures ||
            !ti.langdef)
    {
        /* ti_stop() is never called */
        ti_destroy();
        return -1;
    }

    /*
     * Patch statement `Prio` since the current version of libcleri does
     * not set the GID to `Prio` objects for backward compatibility reasons.
     *
     * TODO: This can be removed in a future release when libcleri sets the
     *       required GID.
     */
    ti.langdef->start->via.sequence            /* START */
        ->olist->next->cl_obj->via.list         /* statements */
        ->cl_obj->via.rule                      /* statement */
        ->cl_obj->gid = CLERI_GID_STATEMENT;    /* prio */

    return 0;
}

void ti_destroy(void)
{
    free(ti.fn);
    free(ti.node_fn);

    ti__stop();

    ti_build_destroy();
    ti_archive_destroy();
    ti_args_destroy();
    ti_cfg_destroy();
    ti_clients_destroy();
    ti_nodes_destroy();
    ti_backups_destroy();
    ti_collections_destroy();
    ti_users_destroy();
    ti_store_destroy();
    ti_val_drop((ti_val_t *) ti.thing0);

    vec_destroy(ti.access_node, (vec_destroy_cb) ti_auth_destroy);
    vec_destroy(ti.access_thingsdb, (vec_destroy_cb) ti_auth_destroy);
    vec_destroy(ti.procedures, (vec_destroy_cb) ti_procedure_destroy);

    /* remove late since counters can be updated */
    ti_counters_destroy();

    /* names must be removed late since they are
     * used by for example procedures */
    ti_names_destroy();

    /* remove late */
    ti_thing_destroy_gc();
    ti_val_drop_common();
    ti_do_drop();

    /* sanity check to see if all references are removed as expected */
    assert(ti_vbool_no_ref());
    assert(ti_nil_no_ref());
    assert(ti_vint_no_ref());

    if (ti.langdef)
        cleri_grammar_free(ti.langdef);
    memset(&ti, 0, sizeof(ti_t));
}

int ti_init_logger(void)
{
    int n;
    char lname[255];
    size_t len = strlen(ti.args->log_level);

#ifdef NDEBUG
    /* force colors while debugging... */
    if (ti.args->log_colorized)
#endif
    {
        Logger.flags |= LOGGER_FLAG_COLORED;
    }

    for (n = 0; n < LOGGER_NUM_LEVELS; n++)
    {
        strcpy(lname, LOGGER_LEVEL_NAMES[n]);
        strx_lower_case(lname);
        if (strlen(lname) == len && strcmp(ti.args->log_level, lname) == 0)
        {
            logger_init(stdout, n);
            return 0;
        }
    }
    assert (0);
    return -1;
}

int ti_init(void)
{
    ti_names_inject_common();
    ti_verror_init();
    ti_qbind_init();

    if (ti.cfg->query_duration_error > ti.cfg->query_duration_warn)
        ti.cfg->query_duration_warn = ti.cfg->query_duration_error;

    if (ti_do_init() || ti_val_init_common() || ti_thing_init_gc())
        return -1;

    ti.fn = strx_cat(ti.cfg->storage_path, ti__fn);
    ti.node_fn = strx_cat(ti.cfg->storage_path, ti__node_fn);

    return (ti.fn && ti.node_fn) ? ti_store_create() : -1;
}



int ti_build_node(void)
{
    char salt[CRYPTX_SALT_SZ];
    char encrypted[CRYPTX_SZ];

    cryptx_gen_salt(salt);
    cryptx("ThingsDB", salt, encrypted);

    ti.node = ti_nodes_new_node(
            0,
            ti.cfg->zone,
            ti.cfg->node_port,
            ti.cfg->node_name,
            encrypted);

    if (!ti.node)
        return -1;

   if (ti_write_node_id(&ti.node->id) ||
       ti_save() ||
       ti_nodes_write_global_status())
       return -1;
   return 0;
}

int ti_build(void)
{
    int rc = -1;
    ti_event_t * ev = NULL;
    if (ti_build_node())
        goto failed;

    ti.node->cevid = 0;
    ti.node->next_thing_id = 1;

    ev = ti_event_initial();
    if (!ev)
        goto failed;

    if (ti_event_run(ev))
        goto failed;

    ti.node->cevid = ev->id;
    ti.node->sevid = ev->id;
    ti.events->next_event_id = ev->id + 1;

    if (ti_store_store())
        goto failed;

    if (ti_archive_rmdir())
        goto failed;

    rc = 0;
    goto done;

failed:
    (void) fx_rmdir(ti.cfg->storage_path);
    (void) ti_store_init();
    ti_node_drop(ti.node);
    ti.node = NULL;
    (void) vec_pop(ti.nodes->vec);

done:
    ti_event_drop(ev);
    return rc;
}

int ti_rebuild(void)
{
    /* remove store path if exists */
    if (fx_is_dir(ti.store->store_path))
    {
        log_warning("removing store directory: `%s`", ti.store->store_path);
        (void) fx_rmdir(ti.store->store_path);
    }

    return ti_store_init() || ti_backups_rm() || ti_archive_rmdir() ? -1 : 0;
}

int ti_write_node_id(uint32_t * node_id)
{
    assert (ti.node_fn);

    int rc = -1;
    FILE * f = fopen(ti.node_fn, "w");
    if (!f)
        goto finish;

    rc = -(fprintf(f, TI_NODE_ID, *node_id) < 0);
    rc = -(fclose(f) || rc);

finish:
    if (rc)
        log_critical("error writing node id to `%s`", ti.node_fn);

    return rc;
}

int ti_read_node_id(uint32_t * node_id)
{
    assert (ti.node_fn);

    unsigned int unode_id = 0;
    int rc = -1;
    FILE * f = fopen(ti.node_fn, "r");
    if (!f)
        goto finish;

    rc = -(fscanf(f, TI_NODE_ID, &unode_id) != 1);
    rc = -(fclose(f) || rc);

    *node_id = (uint32_t) unode_id;

finish:
    if (rc)
        log_critical("error reading node id from `%s`", ti.node_fn);
    else
        log_debug("found node id `%u` in file: `%s`", *node_id, ti.node_fn);

    return rc;
}

int ti_read(void)
{
    int rc;
    ssize_t n;
    uchar * data = fx_read(ti.fn, &n);
    if (!data)
        return -1;

    rc = ti_unpack(data, (size_t) n);
    free(data);
    return rc;
}

int ti_unpack(uchar * data, size_t n)
{
    mp_unp_t up;
    mp_obj_t obj, mp_schema, mp_event_id, mp_next_node_id;
    uint32_t node_id;

    mp_unp_init(&up, data, (size_t) n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 4 ||

        mp_skip(&up) != MP_STR ||  /* schema */
        mp_next(&up, &mp_schema) != MP_U64 ||

        mp_skip(&up) != MP_STR ||  /* event_id */
        mp_next(&up, &mp_event_id) != MP_U64 ||

        mp_skip(&up) != MP_STR ||  /* next_node_id */
        mp_next(&up, &mp_next_node_id) != MP_U64 ||

        mp_skip(&up) != MP_STR ||  /* nodes */
        ti_nodes_from_up(&up)
    ) goto fail;

    if (ti_read_node_id(&node_id))
        goto fail;

    ti.nodes->next_id = mp_next_node_id.via.u64;
    ti.last_event_id = mp_event_id.via.u64;
    ti.node = ti_nodes_node_by_id(node_id);
    if (!ti.node)
        goto fail;

    ti.node->zone = ti.cfg->zone;

    if (ti.node->port != ti.cfg->node_port)
    {
        ti.node->port = ti.cfg->node_port;
        ti.flags |= TI_FLAG_NODES_CHANGED;
    }

    if (strcmp(ti.node->addr, ti.cfg->node_name) != 0)
    {
        char * addr = strdup(ti.cfg->node_name);
        if (addr)
        {
            free(ti.node->addr);
            ti.node->addr = addr;
            ti.flags |= TI_FLAG_NODES_CHANGED;
        }
    }

    return 0;
fail:
    ti.node = NULL;
    log_critical("failed to restore from file: `%s`", ti.fn);
    return -1;
}

static void ti__delayed_start_free(uv_handle_t * UNUSED(timer))
{
    free(delayed_start_timer);
    delayed_start_timer = NULL;
}

static void ti__delayed_start_stop(void)
{
    (void) uv_timer_stop(delayed_start_timer);
    uv_close((uv_handle_t *) delayed_start_timer, ti__delayed_start_free);
}


static void ti__delayed_start_cb(uv_timer_t * UNUSED(timer))
{
    ssize_t n = ti_events_trigger_loop();

    if (n > 0)
    {
        log_info("wait for %zd event%s to load", n, n == 1 ? "" : "s");
        return;
    }

    if (n < 0)
        goto failed;

    if (ti.node)
    {
        if (ti_away_start())
            goto failed;

        if (ti_clients_listen())
            goto failed;

        if (ti_api_init())
            goto failed;

        if (ti_connect_start())
            goto failed;

        if (ti.nodes->vec->n == 1)
        {
            ti.node->status = TI_NODE_STAT_READY;
        }
        else if (ti_sync_start())
            goto failed;
    }

    if (ti_nodes_listen())
        goto failed;

    ti__delayed_start_stop();
    return;

failed:
    ti_stop();
}

int ti_delayed_start(void)
{
    assert (delayed_start_timer == NULL);

    delayed_start_timer = malloc(sizeof(uv_timer_t));
    if (!delayed_start_timer)
        goto fail0;

    if (uv_timer_init(ti.loop, delayed_start_timer))
        goto fail0;

    if (uv_timer_start(delayed_start_timer, ti__delayed_start_cb, 100, 1000))
        goto fail1;

    return 0;

fail1:
    uv_close((uv_handle_t *) delayed_start_timer, (uv_close_cb) free);
fail0:
    free(delayed_start_timer);
    delayed_start_timer = NULL;
    return -1;
}

int ti_run(void)
{
    int rc, attempts;

    if (ti_backups_restore())
        return -1;

    if (uv_loop_init(&loop_))
        return -1;

    ti.loop = &loop_;

    if (ti_signals_init())
        goto failed;

    if (ti.cfg->http_status_port && ti_web_init())
        goto failed;

    if (ti_events_start())
        goto failed;

    if (ti.node)
    {
        ti.node->status = TI_NODE_STAT_SYNCHRONIZING;

        (void) ti_nodes_read_scevid();

        if (ti_archive_init())
            goto failed;

        if (ti_archive_load())
            goto failed;
    }
    else
    {
        if (ti_build_create())
            goto failed;
    }

    ti_delayed_start();

    rc = uv_run(ti.loop, UV_RUN_DEFAULT);
    goto finish;

failed:
    rc = -1;
    ti_stop();

finish:
    attempts = 3;
    while (attempts--)
    {
        rc = uv_loop_close(ti.loop);
        if (!rc)
            break;
        uv_walk(ti.loop, ti__close_handles, NULL);
        (void) uv_run(ti.loop, UV_RUN_NOWAIT);
    }
    return rc;
}

void ti_shutdown(void)
{
    if (ti.node)
        ti_set_and_broadcast_node_status(TI_NODE_STAT_SHUTTING_DOWN);

    assert (shutdown_timer == NULL);

    shutdown_timer = malloc(sizeof(uv_timer_t));
    if (!shutdown_timer)
        goto fail0;

    if (uv_timer_init(ti.loop, shutdown_timer))
        goto fail0;

    if (uv_timer_start(shutdown_timer, ti__shutdown_cb, 250, 1000))
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
    if (ti.node)
    {
        ti_set_and_broadcast_node_status(TI_NODE_STAT_OFFLINE);

        (void) ti_collections_gc();
        (void) ti_archive_to_disk();
        (void) ti_backups_store();
        (void) ti_nodes_write_global_status();
        if (ti.flags & TI_FLAG_NODES_CHANGED)
            (void) ti_save();
    }
    ti__stop();
    uv_stop(ti.loop);
}

int ti_save(void)
{
    msgpack_packer pk;
    FILE * f = fopen(ti.fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, ti.fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (ti.node->cevid > ti.last_event_id)
        ti.last_event_id = ti.node->cevid;

    if (ti_to_pk(&pk))
        goto fail;

    log_debug("stored thingsdb state to file: `%s`", ti.fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", ti.fn);
done:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, ti.fn);
        return -1;
    }
    return 0;
}

int ti_lock(void)
{
    lock_t rc = lock_lock(ti.cfg->storage_path, LOCK_FLAG_OVERWRITE);

    switch (rc)
    {
    case LOCK_IS_LOCKED_ERR:
    case LOCK_PROCESS_NAME_ERR:
    case LOCK_WRITE_ERR:
    case LOCK_READ_ERR:
    case LOCK_MEM_ALLOC_ERR:
        log_error("%s (%s)", lock_str(rc), ti.cfg->storage_path);
        return -1;
    case LOCK_NEW:
        log_debug("%s (%s)", lock_str(rc), ti.cfg->storage_path);
        break;
    case LOCK_OVERWRITE:
        log_warning("%s (%s)", lock_str(rc), ti.cfg->storage_path);
        break;
    default:
        break;
    }
    ti.flags |= TI_FLAG_LOCKED;
    return 0;
}

int ti_unlock(void)
{
    if (ti.flags & TI_FLAG_LOCKED)
    {
        lock_t rc = lock_unlock(ti.cfg->storage_path);
        if (rc != LOCK_REMOVED)
        {
            log_error(lock_str(rc));
            return -1;
        }
    }
    return 0;
}

_Bool ti_ask_continue(const char * warn)
{
    if (ti.args->yes)
        return true;  /* continue without asking */
    printf("\nWarning: %s!!\n\n"
            "Type `yes` + ENTER if you really want to continue: ", warn);
    fflush(stdout);

    if (getchar() != 'y')
        return false;
    if (getchar() != 'e')
        return false;
    if (getchar() != 's')
        return false;
    return getchar() == '\n';
}

void ti_print_connect_info(void)
{
    printf(
        "Waiting for an invite from a node to join ThingsDB...\n"
        "\n"
        "You can use the following query to add this node:\n"
        "\n"
        "    new_node('%s', '%s', %u);\n"
        "\n",
        ti.args->secret,
        ti.cfg->node_name,
        ti.cfg->node_port);
}

ti_rpkg_t * ti_node_status_rpkg(void)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    ti_node_t * ti_node = ti.node;

    if (mp_sbuffer_alloc_init(&buffer, TI_NODE_INFO_PK_SZ, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) ti_node_status_to_pk(ti_node, &pk);

    assert_log(buffer.size < TI_NODE_INFO_PK_SZ, "node info size too small");

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_INFO, buffer.size);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
        free(pkg);

    return rpkg;
}

ti_rpkg_t * ti_client_status_rpkg(void)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    const char * status = ti_node_status_str(ti.node->status);

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    mp_pack_str(&pk, status);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_NODE_STATUS, buffer.size);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
        free(pkg);

    return rpkg;
}

void ti_set_and_broadcast_node_status(ti_node_status_t status)
{
    ti_rpkg_t * client_rpkg;

    if (ti.node->status == status)
        return;  /* node status is not changed */

    log_info("changing status from %s to %s",
            ti_node_status_str(ti.node->status),
            ti_node_status_str(status));

    ti.node->status = status;

    ti_broadcast_node_info();

    client_rpkg = ti_client_status_rpkg();
    if (!client_rpkg)
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    ti_clients_write_rpkg(client_rpkg);
    ti_rpkg_drop(client_rpkg);
}

void ti_set_and_broadcast_node_zone(uint8_t zone)
{
    log_debug("changing zone from %s to %s",
            ti_node_status_str(ti.node->zone),
            ti_node_status_str(zone));

    ti.node->zone = zone;

    ti_broadcast_node_info();
}

void ti_broadcast_node_info(void)
{
    ti_rpkg_t * node_rpkg;

    node_rpkg = ti_node_status_rpkg();
    if (!node_rpkg)
    {
        log_critical(EX_MEMORY_S);
        goto fail;
    }

    ti_nodes_write_rpkg(node_rpkg);

fail:
    ti_rpkg_drop(node_rpkg);
}

int ti_this_node_to_pk(msgpack_packer * pk)
{
    struct timespec timing;
    (void) clock_gettime(TI_CLOCK_MONOTONIC, &timing);
    int yv = yajl_version();

    double uptime = util_time_diff(&ti.boottime, &timing);

    return (
        msgpack_pack_map(pk, 32) ||
        /* 1 */
        mp_pack_str(pk, "node_id") ||
        msgpack_pack_uint32(pk, ti.node->id) ||
        /* 2 */
        mp_pack_str(pk, "version") ||
        mp_pack_str(pk, TI_VERSION) ||
        /* 3 */
        mp_pack_str(pk, "syntax_version") ||
        mp_pack_str(pk, TI_VERSION_SYNTAX_STR) ||
        /* 4 */
        mp_pack_str(pk, "msgpack_version") ||
        mp_pack_str(pk, MSGPACK_VERSION) ||
        /* 5 */
        mp_pack_str(pk, "libcleri_version") ||
        mp_pack_str(pk, cleri_version()) ||
        /* 6 */
        mp_pack_str(pk, "libuv_version") ||
        mp_pack_str(pk, uv_version_string()) ||
        /* 7 */
        mp_pack_str(pk, "libpcre2_version") ||
        mp_pack_str(pk, TI_PCRE2_VERSION) ||
        /* 8 */
        mp_pack_str(pk, "status") ||
        mp_pack_str(pk, ti_node_status_str(ti.node->status)) ||
        /* 9 */
        mp_pack_str(pk, "zone") ||
        msgpack_pack_uint8(pk, ti.node->zone) ||
        /* 10 */
        mp_pack_str(pk, "log_level") ||
        mp_pack_str(pk, Logger.level_name) ||
        /* 11 */
        mp_pack_str(pk, "node_name") ||
        mp_pack_str(pk, ti.cfg->node_name) ||
        /* 12 */
        mp_pack_str(pk, "client_port") ||
        msgpack_pack_uint16(pk, ti.cfg->client_port) ||
        /* 13 */
        mp_pack_str(pk, "node_port") ||
        msgpack_pack_uint16(pk, ti.cfg->node_port) ||
        /* 14 */
        mp_pack_str(pk, "ip_support") ||
        mp_pack_str(
            pk,
            ti_tcp_ip_support_str(ti.cfg->ip_support)) ||
        /* 15 */
        mp_pack_str(pk, "storage_path") ||
        mp_pack_str(pk, ti.cfg->storage_path) ||
        /* 16 */
        mp_pack_str(pk, "uptime") ||
        msgpack_pack_double(pk, uptime) ||
        /* 17 */
        mp_pack_str(pk, "events_in_queue") ||
        msgpack_pack_uint64(pk, ti.events->queue->n) ||
        /* 18 */
        mp_pack_str(pk, "archived_in_memory") ||
        msgpack_pack_uint64(pk, ti.archive->queue->n) ||
        /* 19 */
        mp_pack_str(pk, "archive_files") ||
        msgpack_pack_uint32(pk, ti.archive->archfiles->n) ||
        /* 20 */
        mp_pack_str(pk, "local_stored_event_id") ||
        msgpack_pack_uint64(pk, ti.node->sevid) ||
        /* 21 */
        mp_pack_str(pk, "local_committed_event_id") ||
        msgpack_pack_uint64(pk, ti.node->cevid) ||
        /* 22 */
        mp_pack_str(pk, "global_stored_event_id") ||
        msgpack_pack_uint64(pk, ti_nodes_sevid()) ||
        /* 23 */
        mp_pack_str(pk, "global_committed_event_id") ||
        msgpack_pack_uint64(pk, ti_nodes_cevid()) ||
        /* 24 */
        mp_pack_str(pk, "db_stored_event_id") ||
        msgpack_pack_uint64(pk, ti.store->last_stored_event_id) ||
        /* 25 */
        mp_pack_str(pk, "next_event_id") ||
        msgpack_pack_uint64(pk, ti.events->next_event_id) ||
        /* 26 */
        mp_pack_str(pk, "next_thing_id") ||
        msgpack_pack_uint64(pk, ti.node->next_thing_id) ||
        /* 27 */
        mp_pack_str(pk, "cached_names") ||
        msgpack_pack_uint32(pk, ti.names->n) ||
        /* 28 */
        mp_pack_str(pk, "http_status_port") ||
        (ti.cfg->http_status_port
                ? msgpack_pack_uint16(pk, ti.cfg->http_status_port)
                : mp_pack_str(pk, "disabled")) ||
        /* 29 */
        mp_pack_str(pk, "http_api_port") ||
        (ti.cfg->http_api_port
                ? msgpack_pack_uint16(pk, ti.cfg->http_api_port)
                : mp_pack_str(pk, "disabled")) ||
        /* 30 */
        mp_pack_str(pk, "scheduled_backups") ||
        msgpack_pack_uint64(pk, ti_backups_scheduled()) ||
        /* 31 */
        mp_pack_str(pk, "yajl_version") ||
        mp_pack_fmt(pk, "%d.%d.%d", yv/10000, yv%10000/100, yv%100) ||
        /* 32 */
        mp_pack_str(pk, "connected_clients") ||
        msgpack_pack_uint64(pk, ti_stream_client_connections())
    );
}

ti_val_t * ti_this_node_as_mpval(void)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_this_node_to_pk(&pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
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
    /*
     * The shutdown counter is here so the node might still process queries
     * from other nodes during this time. Other nodes are informed of the
     * shutdown so request should stop in a short period. When there is only
     * one node, there is no point in waiting.
     */
    if (ti.nodes->vec->n > 1 && shutdown_counter)
    {
        log_info("going off-line in %d second%s",
                shutdown_counter, shutdown_counter == 1 ? "" : "s");
        --shutdown_counter;
        return;
    }

    if (ti_away_is_working())
    {
        log_info("wait for `away` mode to finish before shutting down");
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
        /* Stop watching is possible since the last moment before shutting down
         * and starts an a-sync task;
         */
        log_error("non closing a-sync task found; this may leak a few bytes");
        /* fall through */
    case UV_SIGNAL:
        uv_close(handle, NULL);
        break;
    case UV_TCP:
    case UV_NAMED_PIPE:
        if (ti_web_is_handle(handle))
        {
            ti_web_close((ti_web_request_t *) handle->data);
        }
        else if (ti_api_is_handle(handle))
        {
            ti_api_close((ti_api_request_t *) handle->data);
        }
        else if (handle->data)
        {
            ti_stream_close((ti_stream_t *) handle->data);
        }
        else
        {
            uv_close(handle, NULL);
        }
        break;
    case UV_TIMER:
        if (handle == (uv_handle_t *) shutdown_timer)
            ti__shutdown_stop();
        else if (handle == (uv_handle_t *) delayed_start_timer)
            ti__delayed_start_stop();
        else
        {
            log_error("non closing timer found; this may leak a few bytes");
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
