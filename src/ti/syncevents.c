/* ti/syncevents.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/epkg.inline.h>
#include <ti/nodes.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/epkg.h>
#include <ti/syncevents.h>
#include <util/mpack.h>
#include <util/syncpart.h>

static void syncevents__push_cb(ti_req_t * req, ex_enum status);
static void syncevents__done_cb(ti_req_t * req, ex_enum status);


static int syncevents__send(ti_stream_t * stream, ti_epkg_t * epkg)
{
    ti_pkg_t * pkg = ti_pkg_dup(epkg->pkg);
    if (!pkg)
        return -1;

    pkg->id = 0;
    ti_pkg_set_tp(pkg, TI_PROTO_NODE_REQ_SYNCEPART);

    if (ti_req_create(
            stream,
            pkg,
            TI_PROTO_NODE_REQ_SYNCEPART_TIMEOUT,
            syncevents__push_cb,
            NULL))
    {
        free(pkg);
        return -1;
    }

    log_debug(
            "synchronizing "TI_EVENT_ID" to `%s`",
            epkg->event_id,
            ti_stream_name(stream));

    return 0;
}

/* Returns 1 if no package with at least the given `event_id` is not found.
 * and 0 if an event with at least the requested event_id is found and
 * successfully send to the stream.
 * A request with a higher id might be send when there is a gap in the
 * event list.
 * In case of an error, -1 will be the return value.
 */
int ti_syncevents_init(ti_stream_t * stream, uint64_t event_id)
{
    queue_t * events_queue = ti.archive->queue;

    for (queue_each(events_queue, ti_epkg_t, epkg))
        if (epkg->event_id >= event_id)
            return syncevents__send(stream, epkg);

    return 1;
}

ti_pkg_t * ti_syncevents_on_part(ti_pkg_t * pkg, ex_t * e)
{
    int rc;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * resp;
    ti_epkg_t * epkg = ti_epkg_from_pkg(pkg);
    uint64_t next_event_id;

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)) || !epkg)
    {
        ex_set_mem(e);  /* can leak a few bytes */
        return NULL;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    epkg->flags |= TI_EPKG_FLAG_ALLOW_GAP;
    next_event_id = epkg->event_id;
    ti_events_set_next_missing_id(&next_event_id);
    assert (next_event_id > epkg->event_id);

    rc = epkg->event_id > ti.node->cevid
            ? ti_events_add_event(ti.node, epkg)
            : 0;
    ti_epkg_drop(epkg);

    if (rc < 0)
    {
        ex_set_internal(e);  /* can leak a few bytes */
        return NULL;
    }

    msgpack_pack_uint64(&pk, next_event_id);

    resp = (ti_pkg_t *) buffer.data;
    pkg_init(resp, pkg->id, TI_PROTO_NODE_RES_SYNCEPART, buffer.size);

    return resp;
}

int ti_syncevents_done(ti_stream_t * stream)
{
    ti_pkg_t * pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_SYNCEDONE, NULL, 0);

    if (!pkg)
        return -1;

    if (ti_req_create(
            stream,
            pkg,
            TI_PROTO_NODE_REQ_SYNCEDONE_TIMEOUT,
            syncevents__done_cb,
            NULL))
    {
        free(pkg);
        return -1;
    }

    log_debug("synchronizing `%s` is done", ti_stream_name(stream));

    return 0;
}

static void syncevents__push_cb(ti_req_t * req, ex_enum status)
{
    mp_unp_t up;
    ti_pkg_t * pkg = req->pkg_res;
    mp_obj_t mp_event_id;
    uint64_t next_event_id;
    int rc;

    if (status)
        goto failed;

    if (!req->stream)
    {
        log_error("connection to stream lost while synchronizing");
        goto failed;
    }

    if (pkg->tp != TI_PROTO_NODE_RES_SYNCEPART)
    {
        ti_pkg_log(pkg);
        goto failed;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &mp_event_id) != MP_U64)
    {
        log_error("invalid `%s`", ti_proto_str(pkg->tp));
        goto failed;
    }

    next_event_id = mp_event_id.via.u64;

    rc = next_event_id ? ti_syncevents_init(req->stream, next_event_id) : 1;
    if (rc < 0)
    {
        log_error(
                "failed creating request for stream `%s` and "TI_EVENT_ID,
                ti_stream_name(req->stream),
                next_event_id);
        goto failed;
    }

    if (rc > 0 && ti_syncevents_done(req->stream))
        goto failed;

    goto done;

failed:
    ti_stream_stop_watching(req->stream);
done:
    ti_req_destroy(req);
}

static void syncevents__done_cb(ti_req_t * req, ex_enum status)
{
    log_info("finished synchronizing `%s`", ti_stream_name(req->stream));

    if (status)
        log_error("failed response: `%s` (%s)", ex_str(status), status);

    ti_away_syncer_done(req->stream);
    ti_stream_stop_watching(req->stream);

    ti_req_destroy(req);
}


