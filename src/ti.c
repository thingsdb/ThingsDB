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
#include <ti/verror.h>
#include <ti/procedure.h>
#include <ti/version.h>
#include <ifaddrs.h>
#include <ti/web.h>
#include <tiinc.h>
#include <unistd.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/lock.h>
#include <util/mpack.h>
#include <util/strx.h>
#include <util/util.h>

ti_t ti_;

/* settings, nodes etc. */
const char * ti__fn = "ti.mp";
const char * ti__node_fn = ".node";
static int shutdown_counter = 3;
static uv_timer_t * shutdown_timer = NULL;
static uv_loop_t loop_;

static void ti__shutdown_free(uv_handle_t * UNUSED(timer));
static void ti__shutdown_stop(void);
static void ti__shutdown_cb(uv_timer_t * shutdown);
static void ti__close_handles(uv_handle_t * handle, void * arg);
static void ti__stop(void);

int ti_create(void)
{
    ti_.last_event_id = 0;
    ti_.flags = 0;
    ti_.fn = NULL;
    ti_.node_fn = NULL;
    ti_.build = NULL;
    ti_.node = NULL;
    ti_.store = NULL;
    ti_.access_node = vec_new(0);
    ti_.access_thingsdb = vec_new(0);
    ti_.procedures = vec_new(0);
    ti_.langdef = compile_langdef();
    ti_.thing0 = ti_thing_o_create(0, NULL);
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
            !ti_.procedures ||
            !ti_.langdef)
    {
        /* ti_stop() is never called */
        ti_destroy();
        return -1;
    }

    /*
     * Patch statement `Prio` since the current version of libcleri does
     * not set the GID to `Prio` objects for backward compatibility reasons.
     *
     * TODO: this can be removed in a future release when libcleri sets the
     *       required GID.
     */
    ti_.langdef->start->via.sequence            /* START */
        ->olist->next->cl_obj->via.list         /* statements */
        ->cl_obj->via.rule                      /* statement */
        ->cl_obj->gid = CLERI_GID_STATEMENT;    /* prio */

    return 0;
}

void ti_destroy(void)
{
    free(ti_.fn);
    free(ti_.node_fn);

    ti__stop();

    ti_build_destroy();
    ti_archive_destroy();
    ti_args_destroy();
    ti_cfg_destroy();
    ti_clients_destroy();
    ti_nodes_destroy();
    ti_collections_destroy();
    ti_users_destroy();
    ti_store_destroy();
    ti_val_drop((ti_val_t *) ti_.thing0);

    vec_destroy(ti_.access_node, (vec_destroy_cb) ti_auth_destroy);
    vec_destroy(ti_.access_thingsdb, (vec_destroy_cb) ti_auth_destroy);
    vec_destroy(ti_.procedures, (vec_destroy_cb) ti_procedure_destroy);

    /* remove late since counters can be updated */
    ti_counters_destroy();

    /* names must be removed late since they are
     * used by for example procedures */
    ti_names_destroy();

    /* remove late */
    ti_val_drop_common();

    if (ti_.langdef)
        cleri_grammar_free(ti_.langdef);
    memset(&ti_, 0, sizeof(ti_t));
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
    ti_names_inject_common();
    ti_verror_init();

    if (ti_val_init_common())
        return -1;

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
            0,
            ti()->cfg->zone,
            ti_.cfg->node_port,
            "0.0.0.0",
            encrypted);
    if (!ti_.node)
        goto failed;

   if (ti_write_node_id(&ti_.node->id) || ti_save())
       goto failed;

    ti_.node->cevid = 0;
    ti_.node->next_thing_id = 1;

    ev = ti_event_initial();
    if (!ev)
        goto failed;

    if (ti_event_run(ev))
        goto failed;

    ti_.node->cevid = ev->id;
    ti_.node->sevid = ev->id;
    ti_.events->next_event_id = ev->id + 1;

    if (ti_store_store())
        goto failed;

    if (ti_archive_rmdir())
        goto failed;

    rc = 0;
    goto done;

failed:
    (void) fx_rmdir(ti_.cfg->storage_path);
    (void) mkdir(ti_.cfg->storage_path, 0700);
    ti_node_drop(ti_.node);
    ti_.node = NULL;
    (void) imap_pop(ti_.nodes->imap, 0);

done:
    ti_event_drop(ev);
    return rc;
}

int ti_rebuild(void)
{
    /* remove store path if exists */
    if (fx_is_dir(ti_.store->store_path))
    {
        log_warning("removing store directory: `%s`", ti_.store->store_path);
        (void) fx_rmdir(ti_.store->store_path);
    }

    if (mkdir(ti_.store->store_path, 0700))
    {
        log_error("cannot create directory `%s` (%s)",
                ti_.store->store_path,
                strerror(errno));
        return -1;
    }

    return ti_archive_rmdir();
}

int ti_write_node_id(uint32_t * node_id)
{
    assert (ti_.node_fn);

    int rc = -1;
    FILE * f = fopen(ti_.node_fn, "w");
    if (!f)
        goto finish;

    rc = -(fprintf(f, TI_NODE_ID, *node_id) < 0);
    rc = -(fclose(f) || rc);

finish:
    if (rc)
        log_critical("error writing node id to `%s`", ti_.node_fn);

    return rc;
}

int ti_read_node_id(uint32_t * node_id)
{
    assert (ti_.node_fn);

    unsigned int unode_id = 0;
    int rc = -1;
    FILE * f = fopen(ti_.node_fn, "r");
    if (!f)
        goto finish;

    rc = -(fscanf(f, TI_NODE_ID, &unode_id) != 1);
    rc = -(fclose(f) || rc);

    *node_id = (uint32_t) unode_id;

finish:
    if (rc)
        log_critical("error reading node id from `%s`", ti_.node_fn);
    else
        log_debug("found node id `%u` in file: `%s`", *node_id, ti_.node_fn);

    return rc;
}

int ti_read(void)
{
    int rc;
    ssize_t n;
    uchar * data = fx_read(ti_.fn, &n);
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

    ti_.nodes->next_id = mp_next_node_id.via.u64;
    ti_.last_event_id = mp_event_id.via.u64;
    ti_.node = ti_nodes_node_by_id(node_id);
    if (!ti_.node)
        goto fail;

    ti_.node->zone = ti()->cfg->zone;

    if (ti_.node->port != ti()->cfg->node_port)
    {
        ti_.node->port = ti()->cfg->node_port;
        ti_.flags |= TI_FLAG_NODES_CHANGED;
    }

    return 0;
fail:
    ti_.node = NULL;
    log_critical("failed to restore from file: `%s`", ti_.fn);
    return -1;
}

int ti_run(void)
{
    int rc, attempts;

    if (uv_loop_init(&loop_))
        return -1;

    ti_.loop = &loop_;

    if (ti_signals_init())
        goto failed;

    if (ti_.cfg->http_status_port && ti_web_init())
        goto failed;

    if (ti_events_start())
        goto failed;

    if (ti_.node)
    {
        ti_.node->status = TI_NODE_STAT_SYNCHRONIZING;

        (void) ti_nodes_read_scevid();

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

        if (ti()->nodes->imap->n == 1)
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
        (void) ti_nodes_write_global_status();
        if (ti_.flags & TI_FLAG_NODES_CHANGED)
            (void) ti_save();
    }
    ti__stop();
    uv_stop(ti()->loop);
}

int ti_save(void)
{
    msgpack_packer pk;
    FILE * f = fopen(ti_.fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", ti_.fn, strerror(errno));
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (ti_.node->cevid > ti_.last_event_id)
        ti_.last_event_id = ti_.node->cevid;

    if (ti_to_pk(&pk))
        goto fail;

    log_debug("stored thingsdb state to file: `%s`", ti_.fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", ti_.fn);
done:
    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", ti_.fn, strerror(errno));
        return -1;
    }
    return 0;
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

_Bool ti_ask_continue(void)
{
    printf("\nWarning: all data on this node will be removed!!\n\n"
            "Type `yes` + ENTER if you really want to continue: ");

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
    struct ifaddrs * addrs, * tmp;

    printf(
        "Waiting for an invite from a node to join ThingsDB...\n");

    if (getifaddrs(&addrs))
        goto done;

    printf(
        "\n"
        "You can use one of the following queries to add this node:\n");

    for (tmp = addrs; tmp; tmp = tmp->ifa_next)
    {
        if (!tmp->ifa_addr)
            continue;

        if (tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in * addr = (struct sockaddr_in *) tmp->ifa_addr;
            printf(
                "\n"
                "  interface: %s\n"
                "    new_node('%s', '%s', %u);\n",
                tmp->ifa_name,
                ti()->args->secret,
                inet_ntoa(addr->sin_addr),
                ti()->cfg->node_port);
        }
        if (tmp->ifa_addr->sa_family == AF_INET6)
        {
            char ipv6[INET6_ADDRSTRLEN];

            struct sockaddr_in6 * addr = (struct sockaddr_in6 *) tmp->ifa_addr;

            inet_ntop(
                    AF_INET6,
                    &addr->sin6_addr,
                    ipv6,
                    sizeof(ipv6));

            printf(
                "\n"
                "  interface: %s\n"
                "    new_node('%s', '%s', %u);\n",
                tmp->ifa_name,
                ti()->args->secret,
                ipv6,
                ti()->cfg->node_port);
        }
    }

    freeifaddrs(addrs);

done:
    printf(
        "\n"
        "(if you want to create a new ThingsDB instead, press CTRL+C and "
        "use the --init argument)\n");
}

ti_rpkg_t * ti_node_status_rpkg(void)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    ti_node_t * ti_node = ti()->node;

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
    const char * status = ti_node_status_str(ti_.node->status);

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    mp_pack_str(&pk, status);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_CLIENT_NODE_STATUS, buffer.size);

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
        log_critical(EX_MEMORY_S);
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

    double uptime = util_time_diff(&ti_.boottime, &timing);

    return (
        msgpack_pack_map(pk, 28) ||
        /* 1 */
        mp_pack_str(pk, "node_id") ||
        msgpack_pack_uint32(pk, ti_.node->id) ||
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
        mp_pack_str(pk, ti_node_status_str(ti_.node->status)) ||
        /* 9 */
        mp_pack_str(pk, "zone") ||
        msgpack_pack_uint8(pk, ti_.node->zone) ||
        /* 10 */
        mp_pack_str(pk, "log_level") ||
        mp_pack_str(pk, Logger.level_name) ||
        /* 11 */
        mp_pack_str(pk, "hostname") ||
        mp_pack_str(pk, ti_.hostname) ||
        /* 12 */
        mp_pack_str(pk, "client_port") ||
        msgpack_pack_uint16(pk, ti_.cfg->client_port) ||
        /* 13 */
        mp_pack_str(pk, "node_port") ||
        msgpack_pack_uint16(pk, ti_.cfg->node_port) ||
        /* 14 */
        mp_pack_str(pk, "ip_support") ||
        mp_pack_str(
            pk,
            ti_tcp_ip_support_str(ti_.cfg->ip_support)) ||
        /* 15 */
        mp_pack_str(pk, "storage_path") ||
        mp_pack_str(pk, ti_.cfg->storage_path) ||
        /* 16 */
        mp_pack_str(pk, "uptime") ||
        msgpack_pack_double(pk, uptime) ||
        /* 17 */
        mp_pack_str(pk, "events_in_queue") ||
        msgpack_pack_uint64(pk, ti_.events->queue->n) ||
        /* 18 */
        mp_pack_str(pk, "archived_in_memory") ||
        msgpack_pack_uint64(pk, ti_.archive->queue->n) ||
        /* 19 */
        mp_pack_str(pk, "archive_files") ||
        msgpack_pack_uint32(pk, ti_.archive->archfiles->n) ||
        /* 20 */
        mp_pack_str(pk, "local_stored_event_id") ||
        msgpack_pack_uint64(pk, ti_.node->sevid) ||
        /* 21 */
        mp_pack_str(pk, "local_committed_event_id") ||
        msgpack_pack_uint64(pk, ti_.node->cevid) ||
        /* 22 */
        mp_pack_str(pk, "global_stored_event_id") ||
        msgpack_pack_uint64(pk, ti_nodes_sevid()) ||
        /* 23 */
        mp_pack_str(pk, "global_committed_event_id") ||
        msgpack_pack_uint64(pk, ti_nodes_cevid()) ||
        /* 24 */
        mp_pack_str(pk, "db_stored_event_id") ||
        msgpack_pack_uint64(pk, ti_.store->last_stored_event_id) ||
        /* 25 */
        mp_pack_str(pk, "next_event_id") ||
        msgpack_pack_uint64(pk, ti_.events->next_event_id) ||
        /* 26 */
        mp_pack_str(pk, "next_thing_id") ||
        msgpack_pack_uint64(pk, ti_.node->next_thing_id) ||
        /* 27 */
        mp_pack_str(pk, "cached_names") ||
        msgpack_pack_uint32(pk, ti_.names->n) ||
        /* 28 */
        mp_pack_str(pk, "http_status_port") ||
        (ti_.cfg->http_status_port
                ? msgpack_pack_uint16(pk, ti_.cfg->http_status_port)
                : mp_pack_str(pk, "disabled"))
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
        if (ti_web_is_handle(handle))
        {
            ti_web_close((ti_web_request_t *) handle->data);
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
