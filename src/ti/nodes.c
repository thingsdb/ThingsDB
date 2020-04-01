/*
 * nodes.c
 */
#include <assert.h>
#include <stdbool.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/archive.h>
#include <ti/args.h>
#include <ti/auth.h>
#include <ti/away.h>
#include <ti/nodes.h>
#include <ti/fwd.h>
#include <ti/proto.h>
#include <ti/query.inline.h>
#include <ti/syncarchive.h>
#include <ti/syncevents.h>
#include <ti/syncfull.h>
#include <ti/version.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/mpack.h>

#define NODES__UV_BACKLOG 64

typedef ti_pkg_t * (*nodes__part_cb) (ti_pkg_t *, ex_t *);

static ti_node_t * nodes__o[63];    /* other zone */
static ti_node_t * nodes__z[63];    /* same zone */
static ti_nodes_t * nodes;
static ti_nodes_t nodes_;

static const char * nodes__status    = "global_status";

static void nodes__tcp_connection(uv_stream_t * uvstream, int status)
{
    int rc;
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("node connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a TCP node connection");

    stream = ti_stream_create(TI_STREAM_TCP_IN_NODE, &ti_nodes_pkg_cb);
    if (!stream)
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    rc = uv_accept(uvstream, stream->uvstream);
    if (rc)
        goto failed;

    rc = uv_read_start(
            stream->uvstream,
            ti_stream_alloc_buf,
            ti_stream_on_data);
    if (rc)
        goto failed;

    return;

failed:
    log_error("cannot read node TCP stream: `%s`", uv_strerror(rc));
    ti_stream_drop(stream);
}

static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg)
{
    assert (stream->tp == TI_STREAM_TCP_IN_NODE);
    assert (stream->via.node == NULL);

    ti_pkg_t * resp = NULL;
    mp_unp_t up;
    mp_obj_t
        obj,
        mp_this_node_id,
        mp_secret,
        mp_from_node_id,
        mp_from_node_name,
        mp_version,
        mp_min_ver,
        mp_next_thing_id,
        mp_cevid,
        mp_sevid,
        mp_status,
        mp_zone,
        mp_port,
        mp_syntax_ver;

    uint32_t
        this_node_id,
        from_node_id;

    uint8_t
        from_node_status,
        from_node_zone;

    uint16_t
        from_node_syntax_ver,
        from_node_port;

    ti_node_t * node, * this_node = ti.node;
    char * min_ver = NULL;
    char * version = NULL;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 7 ||

        mp_next(&up, &mp_this_node_id) != MP_U64 ||
        mp_next(&up, &mp_secret) != MP_STR ||
        mp_next(&up, &mp_from_node_id) != MP_U64 ||
        mp_next(&up, &mp_from_node_name) != MP_STR ||
        mp_next(&up, &mp_version) != MP_STR ||
        mp_next(&up, &mp_min_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR || obj.via.sz != 7 ||

        mp_next(&up, &mp_next_thing_id) != MP_U64 ||
        mp_next(&up, &mp_cevid) != MP_U64 ||
        mp_next(&up, &mp_sevid) != MP_U64 ||
        mp_next(&up, &mp_status) != MP_U64 ||
        mp_next(&up, &mp_zone) != MP_U64 ||
        mp_next(&up, &mp_port) != MP_U64 ||
        mp_next(&up, &mp_syntax_ver) != MP_U64)
    {
        log_error(
                "invalid connection request from `%s`",
                ti_stream_name(stream));
        return;
    }

    if (mp_secret.via.str.n != CRYPTX_SZ ||
        mp_secret.via.str.data[mp_secret.via.str.n-1] != '\0')
    {
        log_error(
                "invalid secret in request from `%s`",
                ti_stream_name(stream));
        return;
    }

    this_node_id = (uint32_t) mp_this_node_id.via.u64;
    from_node_id = (uint32_t) mp_from_node_id.via.u64;
    from_node_port = (uint16_t) mp_port.via.u64;
    from_node_status = (uint8_t) mp_status.via.u64;
    from_node_zone = (uint8_t) mp_zone.via.u64;
    from_node_syntax_ver = (uint16_t) mp_syntax_ver.via.u64;

    if (from_node_id == this_node_id)
    {
        log_error(
            "got a connection request from `%s` "
            "with the same source and destination: "TI_NODE_ID,
            ti_stream_name(stream),
            from_node_id);
        return;
    }

    version = mp_strdup(&mp_version);
    min_ver = mp_strdup(&mp_min_ver);

    if (!version || !min_ver)
    {
        log_critical(EX_MEMORY_S);
        goto fail;
    }

    if (ti_version_cmp(version, TI_MINIMAL_VERSION) < 0)
    {
        log_error(
            "connection request received from `%s` using version `%s` but at "
            "least version `%s` is required",
            ti_stream_name(stream),
            version,
            TI_MINIMAL_VERSION);
        goto fail;
    }

    if (ti_version_cmp(TI_VERSION, min_ver) < 0)
    {
        log_error(
            "connection request received from `%s` which requires at "
            "least version `%s` but this node is running version `%s`",
            ti_stream_name(stream),
            min_ver,
            TI_VERSION);
        goto fail;
    }

    if (!ti.node)
    {
        assert (*ti.args->secret);
        assert (ti.build);

        if (ti_version_cmp(TI_VERSION, version) < 0)
        {
            log_error(
                "ignore connection request from `%s` because the node is "
                "running on version `%s` while this node version `%s`; "
                "usually this is fine but while building at least the same "
                "version is required",
                ti_stream_name(stream),
                version,
                TI_VERSION);
            goto fail;
        }

        if (ti.build->status == TI_BUILD_REQ_SETUP)
        {
            log_info(
                    "ignore connection request from `%s` since this node is ",
                    "busy building ThingsDB",
                    ti_stream_name(stream));
            goto fail;
        }

        char validate[CRYPTX_SZ];

        cryptx(ti.args->secret, mp_secret.via.str.data, validate);
        if (memcmp(mp_secret.via.str.data, validate, CRYPTX_SZ))
        {
            log_error(
                "connection request received from `%s` with an invalid secret",
                ti_stream_name(stream));
            goto fail;
        }

        (void) ti_build_setup(
                this_node_id,
                from_node_id,
                from_node_status,
                from_node_zone,
                from_node_syntax_ver,
                from_node_port,
                stream);

        if (mp_sbuffer_alloc_init(
                &buffer,
                TI_NODE_INFO_PK_SZ,
                sizeof(ti_pkg_t)))
        {
            log_critical(EX_MEMORY_S);
            goto fail;
        }
        msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

        msgpack_pack_array(&pk, 7);
        msgpack_pack_uint8(&pk, 0);                       /* next_thing_id */
        msgpack_pack_uint8(&pk, 0);                       /* cevid */
        msgpack_pack_uint8(&pk, 0);                       /* sevid */
        msgpack_pack_uint8(&pk, TI_NODE_STAT_BUILDING);   /* status */
        msgpack_pack_uint8(&pk, ti.cfg->zone);         /* zone */
        msgpack_pack_uint16(&pk, ti.cfg->node_port);   /* port */
        msgpack_pack_uint8(&pk, TI_VERSION_SYNTAX);       /* syntax version*/

        goto done;
    }

    if (this_node_id != this_node->id)
    {
        log_error(
            "this is "TI_NODE_ID" but got a connection request "
            "from `%s` who thinks this is "TI_NODE_ID,
            this_node->id,
            ti_stream_name(stream),
            this_node_id);
        goto fail;
    }

    if (memcmp(mp_secret.via.str.data, this_node->secret, CRYPTX_SZ))
    {
        log_error(
            "connection request received from `%s` with an invalid secret",
            ti_stream_name(stream));
        goto fail;
    }

    node = ti_nodes_node_by_id(from_node_id);
    if (!node)
    {
        log_error(
            "cannot accept connection request received from `%s` "
            "because "TI_NODE_ID" is not found",
            ti_stream_name(stream),
            from_node_id);
        goto fail;
    }

    if (node->status > TI_NODE_STAT_CONNECTED)
    {
        /*
         * When only CONNECTED, the connection is not yet authenticated and
         * we might in this case want to switch the connection.
         */
        log_error(
            "cannot accept connection request received from `%s` "
            "because "TI_NODE_ID" is already connected",
            ti_stream_name(stream),
            from_node_id);
        goto fail;
    }

    if (node->stream)
    {
        assert (node->stream->via.node == node);

        if (node->id < this_node->id &&
            node->status != TI_NODE_STAT_CONNECTING)
        {
            log_warning(
                    "connection request from `%s` rejected since a connection "
                    "with "TI_NODE_ID" is already established",
                    ti_stream_name(stream),
                    node->id);
            goto fail;
        }

        log_warning("changing stream for "TI_NODE_ID" from `%s` to `%s",
                node->id,
                ti_stream_name(node->stream),
                ti_stream_name(stream));
        /*
         * We leave the old stream alone since it will be closed once a
         * connection response is received.
         * Instead of dropping the node, we just decrement here so the node
         * might actually drop down to 0 references but the call to
         * ti_stream_set_node(..) below will immediately increase the reference
         * counter by one.
         */
        ti_decref(node);

        node->stream->via.node = NULL;
        node->stream = NULL;
    }

    ti_stream_set_node(stream, node);

    node->status = from_node_status;
    node->zone = from_node_zone;
    node->syntax_ver = from_node_syntax_ver;
    node->cevid = mp_cevid.via.u64;
    node->sevid = mp_sevid.via.u64;
    node->next_thing_id = mp_next_thing_id.via.u64;

    ti_nodes_update_syntax_ver(from_node_zone);

    /* Update node name and/or port if required */
    ti_node_upd_node(node, from_node_port, &mp_from_node_name);

    if (mp_sbuffer_alloc_init(
            &buffer,
            TI_NODE_INFO_PK_SZ,
            sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        goto fail;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) ti_node_status_to_pk(this_node, &pk);

done:
    resp = (ti_pkg_t *) buffer.data;
    pkg_init(resp, pkg->id, TI_PROTO_NODE_RES_CONNECT, buffer.size);

    if (ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_INTERNAL_S);
    }

fail:
    free(version);
    free(min_ver);
}

static void nodes__on_req_event_id(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    mp_unp_t up;
    ti_pkg_t * resp = NULL;
    ti_node_t * other_node = stream->via.node;
    ti_node_t * this_node = ti.node;
    mp_obj_t mp_event_id;
    ti_proto_enum_t accepted;
    uint8_t n = 0;

    if (!this_node)
    {
        ex_set(&e, EX_AUTH_ERROR,
                "got an `%s` request from an unauthorized connection",
                ti_proto_str(pkg->tp));
        goto finish;
    }

    if (this_node->status < TI_NODE_STAT_SHUTTING_DOWN)
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle `%s` requests",
                this_node->id, ti_proto_str(pkg->tp));
        goto finish;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &mp_event_id) != MP_U64)
    {
        ex_set(&e, EX_BAD_DATA,
                "invalid `%s` request from "TI_NODE_ID" to "TI_NODE_ID,
                ti_proto_str(pkg->tp), other_node->id, this_node->id);
        goto finish;
    }

    accepted = ti_events_accept_id(mp_event_id.via.u64, &n);

    log_debug("respond with %s to requested "TI_EVENT_ID" from "TI_NODE_ID,
            ti_proto_str(accepted),
            mp_event_id.via.u64,
            other_node->id);

    if (accepted == TI_PROTO_NODE_ERR_COLLISION)
    {
        msgpack_packer pk;
        msgpack_sbuffer buffer;
        if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)) == 0)
        {
            msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);
            msgpack_pack_uint8(&pk, n);

            resp = (ti_pkg_t *) buffer.data;
            pkg_init(resp, pkg->id, TI_PROTO_NODE_ERR_COLLISION, buffer.size);
            goto finish;
        }
    }

    assert (e.nr == 0);
    resp = ti_pkg_new(pkg->id, accepted, NULL, 0);

finish:
    if (e.nr)
        resp = ti_pkg_client_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_away(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;
    _Bool accepted;

    if (!node)
    {
        ex_set(&e, EX_AUTH_ERROR,
                "got an away request from an unauthorized connection");
        goto finish;
    }

    if (node->status < TI_NODE_STAT_SHUTTING_DOWN)
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle away requests",
                ti.node->id);
        goto finish;
    }

    accepted = ti_away_accept(node->id);

    assert (e.nr == 0);
    resp = ti_pkg_new(
            pkg->id,
            accepted ? TI_PROTO_NODE_RES_ACCEPT : TI_PROTO_NODE_ERR_REJECT,
            NULL,
            0);

finish:
    if (e.nr)
        resp = ti_pkg_client_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    vec_t * access_;
    ti_user_t * user;
    mp_unp_t up;
    ti_pkg_t * resp = NULL;
    ti_query_t * query = NULL;
    ti_node_t * other_node = stream->via.node;
    ti_node_t * this_node = ti.node;
    mp_obj_t obj, mp_user_id, mp_orig;
    ti_scope_t scope;

    if (!other_node)
    {
        ex_set(&e, EX_AUTH_ERROR,
                "got a forwarded query from an unauthorized connection");
        goto finish;
    }

    if (this_node->status <= TI_NODE_STAT_BUILDING)
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle query requests",
                this_node->id);
        goto finish;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_next(&up, &mp_user_id) != MP_U64 ||
        mp_next(&up, &mp_orig) != MP_BIN)
    {
        ex_set(&e, EX_BAD_DATA,
                "invalid query request from "TI_NODE_ID" to "TI_NODE_ID,
                other_node->id, this_node->id);
        goto finish;
    }

    if (ti_scope_init_packed(
            &scope,
            mp_orig.via.bin.data,
            mp_orig.via.bin.n,
            &e))
        goto finish;

    if (scope.tp != TI_SCOPE_NODE &&
        (this_node->status & (
                TI_NODE_STAT_READY |
                TI_NODE_STAT_AWAY_SOON |
                TI_NODE_STAT_SHUTTING_DOWN)) == 0)
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle query requests",
                this_node->id);
        goto finish;
    }

    user = ti_users_get_by_id(mp_user_id.via.u64);

    if (!user)
    {
        ex_set(&e, EX_LOOKUP_ERROR,
                "cannot find "TI_USER_ID" which is used by a query from "
                TI_NODE_ID" to "TI_NODE_ID,
                mp_user_id.via.u64, other_node->id, this_node->id);
        goto finish;
    }

    query = ti_query_create(stream, user, 0);
    if (!query)
    {
        ex_set_mem(&e);
        goto finish;
    }

    if (ti_query_unpack(
            query,
            &scope,
            pkg->id,
            mp_orig.via.bin.data,
            mp_orig.via.bin.n,
            &e))
        goto finish;

    access_ = ti_query_access(query);

    if (ti_access_check_err(access_, query->user, TI_AUTH_READ, &e) ||
        ti_query_parse(query, &e) ||
        ti_query_investigate(query, &e))
        goto finish;

    if (ti_query_will_update(query))
    {
        if (ti_access_check_err(access_, query->user, TI_AUTH_MODIFY, &e) ||
            ti_events_create_new_event(query, &e))
            goto finish;

        return;
    }

    ti_query_run(query);
    return;

finish:
    ti_query_destroy(query);

    if (e.nr)
        resp = ti_pkg_client_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_run(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    vec_t * access_;
    ti_user_t * user;
    mp_unp_t up;
    ti_pkg_t * resp = NULL;
    ti_query_t * query = NULL;
    ti_node_t * other_node = stream->via.node;
    ti_node_t * this_node = ti.node;
    mp_obj_t obj, mp_user_id, mp_orig;
    ti_scope_t scope;

    if (!other_node)
    {
        ex_set(&e, EX_AUTH_ERROR,
                "got a forwarded run request from an unauthorized connection");
        goto finish;
    }

    if ((this_node->status & (
            TI_NODE_STAT_READY |
            TI_NODE_STAT_AWAY_SOON |
            TI_NODE_STAT_SHUTTING_DOWN)) == 0)
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle `run` requests",
                this_node->id);
        goto finish;
    }

    mp_unp_init(&up, pkg->data, pkg->n);


    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_next(&up, &mp_user_id) != MP_U64 ||
        mp_next(&up, &mp_orig) != MP_BIN)
    {
        ex_set(&e, EX_BAD_DATA,
                "invalid run request from "TI_NODE_ID" to "TI_NODE_ID,
                other_node->id, this_node->id);
        goto finish;
    }

    user = ti_users_get_by_id(mp_user_id.via.u64);
    if (!user)
    {
        ex_set(&e, EX_LOOKUP_ERROR,
                "cannot find "TI_USER_ID" which is used by a call from "
                TI_NODE_ID" to "TI_NODE_ID,
                mp_user_id.via.u64, other_node->id, this_node->id);
        goto finish;
    }

    query = ti_query_create(stream, user, 0);
    if (!query)
    {
        ex_set_mem(&e);
        goto finish;
    }

    if (ti_scope_init_packed(
            &scope,
            mp_orig.via.bin.data,
            mp_orig.via.bin.n,
            &e) ||
        ti_query_unp_run(
                query,
                &scope,
                pkg->id,
                mp_orig.via.bin.data,
                mp_orig.via.bin.n,
                &e))
        goto finish;

    access_ = ti_query_access(query);
    assert (access_);

    if (ti_access_check_err(access_, query->user, TI_AUTH_RUN, &e))
        goto finish;

    if (ti_query_will_update(query))
    {
        if (ti_events_create_new_event(query, &e))
            goto finish;

        return;
    }

    ti_query_run(query);
    return;

finish:
    ti_query_destroy(query);

    if (e.nr)
        resp = ti_pkg_client_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_setup(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error("got a setup request from an unauthorized connection");
        return;
    }

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_pkg_t), sizeof(ti_pkg_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_to_pk(&pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        log_critical(EX_MEMORY_S);
        return;
    }

    resp = (ti_pkg_t *) buffer.data;
    pkg_init(resp, pkg->id, TI_PROTO_NODE_RES_SETUP, buffer.size);

    if (ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_critical(EX_MEMORY_S);
    }
}

static void nodes__on_req_sync(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;
    mp_unp_t up;
    mp_obj_t mp_start;

    if (!node)
    {
        log_error("got a sync request from an unauthorized connection");
        return;
    }

    if ((ti.node->status & (TI_NODE_STAT_AWAY_SOON|TI_NODE_STAT_AWAY)) == 0)
    {
        log_error(
                "got a sync request from `%s` "
                "but this node is not in `away` mode",
                ti_stream_name(stream));
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not in `away` mode and therefore cannot handle "
                "sync requests",
                ti.node->id);
        goto finish;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &mp_start) != MP_U64)
    {
        log_error(
                "got an invalid sync request from `%s`",
                ti_stream_name(stream));
        ex_set(&e, EX_BAD_DATA, "invalid sync request");
        goto finish;
    }

    if (ti_away_syncer(stream, mp_start.via.u64))
    {
        ex_set_mem(&e);
        goto finish;
    }

    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNC, NULL, 0);

finish:
    if (e.nr)
        resp = ti_pkg_node_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_syncpart(
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        nodes__part_cb part_cb)
{
    ex_t e = {0};
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti.node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));

        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti.node->id);
        goto finish;
    }

    resp = part_cb(pkg, &e);
    assert (!resp ^ !e.nr);

finish:
    if (e.nr)
        resp = ti_pkg_node_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_syncfdone(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti.node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti.node->id);
        goto finish;
    }

    (void) ti_store_restore();
    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNCFDONE, NULL, 0);

finish:
    if (e.nr)
        resp = ti_pkg_node_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_syncadone(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti.node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti.node->id);
        goto finish;
    }

    (void) ti_archive_load();

    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNCADONE, NULL, 0);

finish:
    if (e.nr)
        resp = ti_pkg_node_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_req_syncedone(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti.node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti.node->id);
        goto finish;
    }

    ti_sync_stop();
    ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);

    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNCEDONE, NULL, 0);

finish:
    if (e.nr)
        resp = ti_pkg_node_err(pkg->id, &e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void nodes__on_event(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_node_t * other_node = stream->via.node;

    if (!other_node)
    {
        log_error(
                "got a `%s` from an unauthorized connection: `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    ti_events_on_event(other_node, pkg);
}

static void nodes__on_info(ti_stream_t * stream, ti_pkg_t * pkg)
{
    mp_unp_t up;
    ti_node_t * other_node = stream->via.node;

    if (!other_node)
    {
        log_error(
                "got a `%s` from an unauthorized connection: `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (ti_node_status_from_unp(other_node, &up))
    {
        log_error("invalid `%s` from `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }
}

static void nodes__on_missing_event(ti_stream_t * stream, ti_pkg_t * pkg)
{
    mp_unp_t up;
    ti_node_t * other_node = stream->via.node;
    mp_obj_t mp_id;
    ti_epkg_t * epkg;

    if (!other_node)
    {
        log_error(
                "got a `%s` from an unauthorized connection: `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &mp_id) != MP_U64)
    {
        log_error("got an invalid missing event package");
        return;
    }

    epkg = ti_archive_get_event(mp_id.via.u64);
    if (epkg)
        (void) ti_stream_write_rpkg(stream, (ti_rpkg_t *) epkg);

    log_info("%s missing "TI_EVENT_ID"; (request from "TI_NODE_ID")",
            epkg ? "respond with event to" : "cannot find",
            mp_id.via.u64,
            other_node->id);
}

static void nodes__on_fwd_wu(
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        const char * action)  /* action = "watch" or "unwatch" */
{
    ti_thing_t * thing;
    ti_collection_t * collection;
    ti_fwd_t * fwd;
    ti_req_t * req;
    mp_unp_t up;
    ti_node_t * other_node = stream->via.node;
    mp_obj_t obj, mp_cid, mp_tid;

    if (!other_node)
    {
        log_error(
                "got a `%s` from an unauthorized connection: `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_next(&up, &mp_cid) != MP_U64 ||
        mp_next(&up, &mp_tid) != MP_U64)
    {
        log_error("got an invalid (forwarded) `%s` package", action);
        return;
    }

    req = omap_get(stream->reqmap, pkg->id);
    if (!req || !ti_clients_is_fwd_req(req))
    {
        if (req && ti_api_is_fwd_req(req))
        {
            log_debug(
                "function `%s()` with a HTTP API request has no effect",
                action);
        }
        else
        {
            log_warning("unexpected or lost request (package id %u)", pkg->id);
        }
        return;
    }

    fwd = req->data;
    collection = ti_collections_get_by_id(mp_cid.via.u64);
    if (!collection)
    {
        log_warning("cannot find "TI_COLLECTION_ID, mp_cid.via.u64);
        return;
    }

    uv_mutex_lock(collection->lock);

    thing = imap_get(collection->things, mp_tid.via.u64);
    if (thing)
    {
        int rc = *action == 'w'
                ? ti_thing_watch_init(thing, fwd->stream)
                : ti_thing_unwatch(thing, fwd->stream);
        if (rc)
            log_error("%s() has failed (%d)", action, rc);
    }
    else
    {
        log_warning(
                "cannot find "TI_THING_ID" in "TI_COLLECTION_ID,
                mp_tid.via.u64, mp_cid.via.u64);
    }

    uv_mutex_unlock(collection->lock);
}

static const char * nodes__get_status_fn(void)
{
    if (nodes->status_fn)
        return nodes->status_fn;

    nodes->status_fn = fx_path_join(ti.cfg->storage_path, nodes__status);
    return nodes->status_fn;
}

int ti_nodes_create(void)
{
    nodes = &nodes_;

    /* make sure data is set to null, we use this on close */
    nodes->tcp.data = NULL;
    nodes->cevid = 0;
    nodes->sevid = 0;
    nodes->status_fn = NULL;
    nodes->next_id = 0;

    nodes->vec = vec_new(3);
    ti.nodes = nodes;

    return -(nodes->vec == NULL);
}

void ti_nodes_destroy(void)
{
    if (!nodes)
        return;
    vec_destroy(nodes->vec, (vec_destroy_cb) ti_node_drop);
    free(nodes->status_fn);
    ti.nodes = nodes = NULL;
}

int ti_nodes_read_scevid(void)
{
    int rc = -1;
    uint64_t cevid, sevid;
    const char * fn = nodes__get_status_fn();
    FILE * f;

    if (!fn)
    {
        log_critical(EX_INTERNAL_S);
        return -1;
    }

    f = fopen(fn, "r");
    if (!f)
    {
        char ebuf[512];
        log_debug(
                "cannot open file `%s` (%s)",
                fn, log_strerror(errno, ebuf, sizeof(ebuf)));
        return -1;
    }

    if (fread(&cevid, sizeof(uint64_t), 1, f) != 1 ||
        fread(&sevid, sizeof(uint64_t), 1, f) != 1)
    {
        log_error("error reading global event status from: `%s`", fn);
        goto stop;
    }

    log_debug("known committed on all nodes: "TI_EVENT_ID, cevid);
    log_debug("known stored on all nodes: "TI_EVENT_ID, sevid);

    ti.nodes->cevid = cevid;
    ti.nodes->sevid = sevid;

    rc = 0;

stop:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        rc = -1;
    }
    return rc;
}

int ti_nodes_write_global_status(void)
{
    int rc = 0;
    uint64_t cevid = ti_nodes_cevid();
    uint64_t sevid = ti_nodes_sevid();
    const char * fn = nodes__get_status_fn();
    FILE * f;

    if (!fn)
    {
        log_critical(EX_INTERNAL_S);
        return -1;
    }

    f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    log_debug(
            "save global committed "TI_EVENT_ID", "
            "global stored "TI_EVENT_ID" and "
            "lowest known "TI_QBIND" to disk",
            cevid, sevid, nodes->syntax_ver);

    if (fwrite(&cevid, sizeof(uint64_t), 1, f) != 1 ||
        fwrite(&sevid, sizeof(uint64_t), 1, f) != 1)
    {
        log_error("error writing to `%s`", fn);
        rc = -1;
    }

    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        rc = -1;
    }
    return rc;
}

/*
 * Number of nodes required, `this` node excluded.
 */
uint8_t ti_nodes_quorum(void)
{
    if (nodes->vec->n == 2)
    {
        /* we have a special case when there are only two nodes.
         * usually we want to calculate the quorum by simply dividing the
         * number of nodes by two, but it only two nodes exists, and the
         * second node is unreachable, we would never have a chance to do
         * anything since no event could be created.
         */
        for (vec_each(nodes->vec, ti_node_t, node))
            if (node->status <= TI_NODE_STAT_SHUTTING_DOWN)
                return 0;
    }
    return (uint8_t) (nodes->vec->n / 2);
}

_Bool ti_nodes_has_quorum(void)
{
    size_t quorum = ti_nodes_quorum() + 1;  /* include `this` node */
    size_t q = 0;
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status > TI_NODE_STAT_SHUTTING_DOWN && ++q == quorum)
            return true;
    return false;
}

ti_node_t * ti_nodes_not_ready(void)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status != TI_NODE_STAT_READY)
            return node;
    return NULL;
}

/* increases with a new reference as long as required */
void ti_nodes_write_rpkg(ti_rpkg_t * rpkg)
{
    ti_node_t * this_node = ti.node;
    vec_t * nodes_vec = nodes->vec;
    for (vec_each(nodes_vec, ti_node_t, node))
    {
        ti_node_status_t status = node->status;

        if (node == this_node)
            continue;

        if ((status & (
                TI_NODE_STAT_READY |
                TI_NODE_STAT_AWAY_SOON |
                TI_NODE_STAT_AWAY |
                TI_NODE_STAT_SYNCHRONIZING
            )) && ti_stream_write_rpkg(node->stream, rpkg)
        ) log_error(EX_INTERNAL_S);
    }
}

int ti_nodes_to_pk(msgpack_packer * pk)
{
    vec_t * nodes_vec = nodes->vec;
    if (msgpack_pack_array(pk, nodes_vec->n))
        return -1;

    for (vec_each(nodes_vec, ti_node_t, node))
    {
        if (msgpack_pack_array(pk, 5) ||
            msgpack_pack_uint32(pk, node->id) ||
            msgpack_pack_uint8(pk, node->zone) ||
            msgpack_pack_uint16(pk, node->port) ||
            mp_pack_str(pk, node->addr) ||
            mp_pack_strn(pk, node->secret, CRYPTX_SZ)
        ) return -1;
    }

    return 0;
}

int ti_nodes_from_up(mp_unp_t * up)
{
    size_t i;
    mp_obj_t obj, mp_id, mp_zone, mp_port, mp_addr, mp_secret;
    if (mp_next(up, &obj) != MP_ARR)
        return -1;

    for (i = obj.via.sz; i--;)
    {
        char addr[INET6_ADDRSTRLEN];

        if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 5 ||

            mp_next(up, &mp_id) != MP_U64 ||
            mp_next(up, &mp_zone) != MP_U64 ||
            mp_next(up, &mp_port) != MP_U64 ||
            mp_next(up, &mp_addr) != MP_STR ||
            mp_next(up, &mp_secret) != MP_STR
        ) return -1;

        if (mp_addr.via.str.n >= INET6_ADDRSTRLEN)
            return -1;

        if (mp_secret.via.str.n != CRYPTX_SZ)
            return -1;

        memcpy(addr, mp_addr.via.str.data, mp_addr.via.str.n);
        addr[mp_addr.via.str.n] = '\0';

        if (!ti_nodes_new_node(
                mp_id.via.u64,
                mp_zone.via.u64,
                mp_port.via.u64,
                addr,
                mp_secret.via.str.data)
        ) return -1;
    }
    return 0;
}

ti_nodes_ignore_t ti_nodes_ignore_sync(uint8_t retry_offline)
{
    uint64_t m = ti.node->cevid;
    uint8_t n = 0, offline = 0;

    if (!m)
        return TI_NODES_WAIT_AWAY;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (node->cevid > m || node->status > TI_NODE_STAT_SYNCHRONIZING)
            return TI_NODES_WAIT_AWAY;

        if (retry_offline && node->status < TI_NODE_STAT_SYNCHRONIZING)
            ++offline;

        if (node->status == TI_NODE_STAT_SYNCHRONIZING)
            ++n;
    }

    return offline
            ? TI_NODES_RETRY_OFFLINE
            : n > ti_nodes_quorum()
            ? TI_NODES_IGNORE_SYNC
            : TI_NODES_WAIT_AWAY;
}

_Bool ti_nodes_require_sync(void)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status == TI_NODE_STAT_SYNCHRONIZING)
            return true;
    return false;
}

/* Returns 0 if a new node can be added depending on the status of the current
 * nodes. The rule is that at least a quorum can still be reached, even if the
 * new node fails to connect.
 */
int ti_nodes_check_add(ex_t * e)
{
    vec_t * nodes_vec = nodes->vec;
    uint8_t may_skip = nodes_vec->n >= 4
            ? ((uint8_t) (nodes_vec->n / 2)) - 1
            : 0;

    for (vec_each(nodes_vec, ti_node_t, node))
    {
        if (node->status <= TI_NODE_STAT_CONNECTED && !may_skip--)
        {
            ex_set(e, EX_OPERATION_ERROR,
                "wait for a connection to "TI_NODE_ID" before adding a new node; "
                "current status: `%s`",
                node->id, ti_node_status_str(node->status));
            return e->nr;
        }
    }
    return 0;
}

uint64_t ti_nodes_cevid(void)
{
    uint64_t m = ti.node->cevid;
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->cevid < m)
            m = node->cevid;

    if (m > nodes->cevid)
        nodes->cevid = m;

    return nodes->cevid;
}

uint64_t ti_nodes_sevid(void)
{
    uint64_t m = ti.node->sevid;
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->sevid < m)
            m = node->sevid;

    if (m > nodes->sevid)
        nodes->sevid = m;

    return nodes->sevid;
}

uint32_t ti_nodes_next_id(void)
{
    return nodes->next_id;
}

void ti_nodes_update_syntax_ver(uint16_t syntax_ver)
{
    if (syntax_ver == nodes->syntax_ver)
        return;

    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->syntax_ver < syntax_ver)
            syntax_ver = node->syntax_ver;

    if (syntax_ver < nodes->syntax_ver)
    {
        log_error(
                "new "TI_QBIND" is older than the current "TI_QBIND,
                syntax_ver, nodes->syntax_ver);
        nodes->syntax_ver = syntax_ver;
        return;
    }

    if (syntax_ver > nodes->syntax_ver)
        nodes->syntax_ver = syntax_ver;
}


ti_node_t * ti_nodes_new_node(
        uint32_t id,
        uint8_t zone,
        uint16_t port,
        const char * addr,
        const char * secret)
{
    ti_node_t * node = ti_node_create(id, zone, port, addr, secret);
    if (!node || vec_push(&nodes->vec, node))
    {
        ti_node_drop(node);
        return NULL;
    }
    if (id >= nodes->next_id)
        nodes->next_id = id + 1;
    return node;
}

void ti_nodes_del_node(uint32_t node_id)
{
    size_t idx = 0;
    for (vec_each(nodes->vec, ti_node_t, node), ++idx)
    {
        if (node->id == node_id)
        {
            ti_node_drop(vec_swap_remove(nodes->vec, idx));
            return;
        }
    }
}

/*
 * Returns a weak reference to a node, of NULL if not found
 */
ti_node_t * ti_nodes_node_by_id(uint32_t node_id)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->id == node_id)
            return node;
    return NULL;
}

int ti_nodes_listen(void)
{
    struct sockaddr_storage addr = {0};
    int rc;
    ti_cfg_t * cfg = ti.cfg;
    _Bool is_ipv6 = false;
    char * ip;

    uv_tcp_init(ti.loop, &nodes->tcp);

    if (cfg->bind_node_addr != NULL)
    {
        struct in6_addr sa6;
        if (inet_pton(AF_INET6, cfg->bind_node_addr, &sa6))
        {
            is_ipv6 = true;
        }
        ip = cfg->bind_node_addr;
    }
    else if (cfg->ip_support == AF_INET)
    {
        ip = "0.0.0.0";
    }
    else
    {
        ip = "::";
        is_ipv6 = true;
    }

    if (is_ipv6)
    {
        if (uv_ip6_addr(ip, cfg->node_port, (struct sockaddr_in6 *) &addr))
        {
            log_error(
                    "cannot create IPv6 address from `[%s]:%d`",
                    ip, cfg->node_port);
            return -1;
        }
    }
    else
    {
        if (uv_ip4_addr(ip, cfg->node_port, (struct sockaddr_in *) &addr))
        {
            log_error(
                    "cannot create IPv4 address from `%s:%d`",
                    ip, cfg->node_port);
            return -1;
        }
    }

    if ((rc = uv_tcp_bind(
            &nodes->tcp,
            (const struct sockaddr *) &addr,
            (cfg->ip_support == AF_INET6) ?
                    UV_TCP_IPV6ONLY : 0)) ||
        (rc = uv_listen(
            (uv_stream_t *) &nodes->tcp,
            NODES__UV_BACKLOG,
            nodes__tcp_connection)))
    {
        log_error("error listening for node connections on TCP port %d: `%s`",
                cfg->node_port,
                uv_strerror(rc));
        return -1;
    }

    log_info("start listening for node connections on TCP port %d",
            cfg->node_port);

    return 0;
}

/*
 * Returns a borrowed node in away mode or NULL if none is found
 */
ti_node_t * ti_nodes_get_away(void)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status == TI_NODE_STAT_AWAY)
            return node;
    return NULL;
}

/*
 * Returns a borrowed node in away or soon mode or NULL if none is found
 */
ti_node_t * ti_nodes_get_away_or_soon(void)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status == TI_NODE_STAT_AWAY ||
            node->status == TI_NODE_STAT_AWAY_SOON)
            return node;
    return NULL;
}

/*
 * Returns another borrowed node with status READY if possible from the same
 * zone of NULL if no ready node is found. (not thread safe)
 */
ti_node_t * ti_nodes_random_ready_node(void)
{
    ti_node_t * this_node = ti.node;
    uint32_t zn = 0, on = 0;
    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (node == this_node || node->status != TI_NODE_STAT_READY)
            continue;

        if (this_node->zone == node->zone)
            nodes__z[zn++] = node;
        else
            nodes__o[on++] = node;
    }
    return zn ? nodes__z[rand() % zn] : on ? nodes__o[rand() % on] : NULL;
}

void ti_nodes_set_not_ready_err(ex_t * e)
{
    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (node->status == TI_NODE_STAT_SYNCHRONIZING)
        {
            ex_set(e, EX_NODE_ERROR,
                "cannot find a node for handling this request; "
                "please wait until "TI_NODE_ID" has finished synchronizing",
                node->id);
            return;
        }

        if (node->status == TI_NODE_STAT_BUILDING)
        {
            ex_set(e, EX_NODE_ERROR,
                "cannot find a node for handling this request; "
                "please wait until "TI_NODE_ID" has finished building ThingsDB",
                node->id);
            return;
        }

        if (node->status == TI_NODE_STAT_OFFLINE || (node->status & (
                TI_NODE_STAT_CONNECTING |
                TI_NODE_STAT_CONNECTED |
                TI_NODE_STAT_SHUTTING_DOWN)))
        {
            ex_set(e, EX_NODE_ERROR,
                "cannot find a node for handling this request; "
                "at least "TI_NODE_ID" is unreachable, is it turned off?",
                node->id);
            return;
        }
    }
    ex_set(e, EX_NODE_ERROR, "cannot find a node for handling this request");
}

void ti_nodes_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg)
{
    switch (pkg->tp)
    {
    case TI_PROTO_CLIENT_RES_DATA :
    case TI_PROTO_CLIENT_RES_ERROR:
        ti_stream_on_response(stream, pkg);
        break;
    case TI_PROTO_NODE_EVENT:
        nodes__on_event(stream, pkg);
        break;
    case TI_PROTO_NODE_INFO:
        nodes__on_info(stream, pkg);
        break;
    case TI_PROTO_NODE_MISSING_EVENT:
        nodes__on_missing_event(stream, pkg);
        break;
    case TI_PROTO_NODE_FWD_WATCH:
        nodes__on_fwd_wu(stream, pkg, "watch");
        break;
    case TI_PROTO_NODE_FWD_UNWATCH:
        nodes__on_fwd_wu(stream, pkg, "unwatch");
        break;
    case TI_PROTO_NODE_REQ_QUERY:
        nodes__on_req_query(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_RUN:
        nodes__on_req_run(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_CONNECT:
        nodes__on_req_connect(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_EVENT_ID:
        nodes__on_req_event_id(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_AWAY:
        nodes__on_req_away(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SETUP:
        nodes__on_req_setup(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNC:
        nodes__on_req_sync(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNCFPART:
        nodes__on_req_syncpart(stream, pkg, ti_syncfull_on_part);
        break;
    case TI_PROTO_NODE_REQ_SYNCFDONE:
        nodes__on_req_syncfdone(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNCAPART:
        nodes__on_req_syncpart(stream, pkg, ti_syncarchive_on_part);
        break;
    case TI_PROTO_NODE_REQ_SYNCADONE:
        nodes__on_req_syncadone(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNCEPART:
        nodes__on_req_syncpart(stream, pkg, ti_syncevents_on_part);
        break;
    case TI_PROTO_NODE_REQ_SYNCEDONE:
        nodes__on_req_syncedone(stream, pkg);
        break;
    case TI_PROTO_NODE_RES_CONNECT:
    case TI_PROTO_NODE_RES_ACCEPT:
    case TI_PROTO_NODE_RES_SETUP:
    case TI_PROTO_NODE_RES_SYNC:
    case TI_PROTO_NODE_RES_SYNCFPART:
    case TI_PROTO_NODE_RES_SYNCFDONE:
    case TI_PROTO_NODE_RES_SYNCAPART:
    case TI_PROTO_NODE_RES_SYNCADONE:
    case TI_PROTO_NODE_RES_SYNCEPART:
    case TI_PROTO_NODE_RES_SYNCEDONE:
    case TI_PROTO_NODE_ERR_RES:
    case TI_PROTO_NODE_ERR_REJECT:
    case TI_PROTO_NODE_ERR_COLLISION:
        ti_stream_on_response(stream, pkg);
        break;
    default:
        log_error(
                "got an unexpected package type %u from `%s`",
                pkg->tp,
                ti_stream_name(stream));
    }
}

ti_varr_t * ti_nodes_info(void)
{
    ti_varr_t * varr = ti_varr_create(nodes->vec->n);
    if (!varr)
        return NULL;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        ti_val_t * mpinfo = ti_node_as_mpval(node);
        if (!mpinfo)
        {
            ti_val_drop((ti_val_t *) varr);
            return NULL;
        }
        VEC_push(varr->vec, mpinfo);
    }
    return varr;
}

int ti_nodes_check_syntax(uint16_t syntax_ver, ex_t * e)
{
    if (nodes_.syntax_ver >= syntax_ver)
        return 0;
    ex_set(e, EX_SYNTAX_ERROR,
            "not all nodes are running the required "TI_QBIND, syntax_ver);
    return e->nr;
}
