/*
 * ti/syncevents.c
 */
#include <assert.h>
#include <ti/proto.h>
#include <ti.h>
#include <ti/nodes.h>
#include <ti/syncevents.h>
#include <util/qpx.h>
#include <util/syncpart.h>

#define SYNCFULL__PART_SIZE 131072UL

static ti_pkg_t * syncevents__pkg(ti_archfile_t * archfile, off_t offset);
static void syncevents__push_cb(ti_req_t * req, ex_enum status);
static void syncevents__done_cb(ti_req_t * req, ex_enum status);

/*
 * Returns 1 if the given `event_id` is not found and 0 if it is found
 * and a request is successfully made. In case of an error, -1
 * will be the return value.
 */
int ti_syncevents_init(ti_stream_t * stream, uint64_t event_id)
{
    queue_t * event_pkgs = ti()->archive->archfiles;

    for (queue_each(event_pkgs, ti_epkg_t, epkg))
    {
        if (epkg->event_id == event_id)
        {
            ti_pkg_t * pkg = ti_pkg_dup(epkg->pkg);
            if (!pkg)
                return -1;

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
            return 0;
        }
    }

    return 1;
}

ti_pkg_t * ti_syncevents_on_part(ti_pkg_t * pkg, ex_t * e)
{
    int rc;
    qp_unpacker_t unpacker;
    ti_pkg_t * resp;
    qp_obj_t qp_first, qp_last, qp_offset, qp_raw, qp_more;
    qpx_packer_t * packer;
    off_t offset;
    uint64_t first, last;
    ti_archfile_t * archfile;

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_first)) ||
        !qp_is_int(qp_next(&unpacker, &qp_last)) ||
        !qp_is_int(qp_next(&unpacker, &qp_offset)) ||
        !qp_is_raw(qp_next(&unpacker, &qp_raw)) ||
        !qp_is_bool(qp_next(&unpacker, &qp_more)))
    {
        ex_set(e, EX_BAD_DATA, "invalid multipart request");
        return NULL;
    }

    first = (uint64_t) qp_first.via.int64;
    last = (uint64_t) qp_last.via.int64;
    offset = (off_t) qp_offset.via.int64;

    archfile = ti_archfile_get(first, last);

    if (!archfile)
    {
        ti_archive_t * archive = ti()->archive;

        if (offset)
        {
            ex_set(e, EX_BAD_DATA,
                    "missing archive file for event range %"PRIx64"-%"PRIx64,
                    first, last);
            return NULL;
        }

        archfile = ti_archfile_from_event_ids(archive->path, first, last);
        if (!archfile || queue_push(&archive->archfiles, archfile))
        {
            ti_archfile_destroy(archfile);
            ex_set_alloc(e);
            return NULL;
        }
    }

    rc = syncpart_write(archfile->fn, qp_raw.via.raw, qp_raw.len, offset, e);
    if (rc)
        return NULL;

    packer = qpx_packer_create(48, 1);
    if (!packer)
    {
        ex_set_alloc(e);
        return NULL;
    }

    if (qp_more.tp == QP_TRUE)
    {
        offset += qp_raw.len;
    }
    else
    {
//        ti_event_t * first_event;

//        if (ti_archive_load_file(archfile))
//        {
//            ex_set(e, EX_INTERNAL,
//                    "error loading archive file `%s`",
//                    archfile->fn);
//            return NULL;
//        }

//        first_event = queue_first(ti()->events->queue);

        offset = 0;
        first = 0; // first_event ? first_event->id : 0;
        last = archfile->last + 1;
    }

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, first);
    (void) qp_add_int(packer, last);
    (void) qp_add_int(packer, offset);
    (void) qp_close_array(packer);

    resp = qpx_packer_pkg(packer, TI_PROTO_NODE_RES_SYNCEPART);
    resp->id = pkg->id;

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
    return 0;
}

static ti_pkg_t * syncevents__pkg(ti_archfile_t * archfile, off_t offset)
{
    int more;
    qpx_packer_t * packer = qpx_packer_create(48 + SYNCPART_SIZE, 1);
    if (!packer)
        return NULL;

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, archfile->first);
    (void) qp_add_int(packer, archfile->last);
    (void) qp_add_int(packer, offset);              /* offset in file */

    more = syncpart_to_packer(packer, archfile->fn, offset);
    if (more < 0)
        goto failed;

    (void) qp_add_bool(packer, (_Bool) more);
    (void) qp_close_array(packer);

    return qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_SYNCEPART);

failed:
    qp_packer_destroy(packer);
    return NULL;
}

static void syncevents__push_cb(ti_req_t * req, ex_enum status)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * pkg = req->pkg_res;
    ti_pkg_t * next_pkg;
    qp_obj_t qp_first, qp_last, qp_offset;
    off_t offset;
    uint64_t first, last;
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

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_first)) ||
        !qp_is_int(qp_next(&unpacker, &qp_last)) ||
        !qp_is_int(qp_next(&unpacker, &qp_offset)))
    {
        log_error("invalid `%s`", ti_proto_str(pkg->tp));
        goto failed;
    }

    first = (uint64_t) qp_first.via.int64;
    last = (uint64_t) qp_last.via.int64;
    offset = (off_t) qp_offset.via.int64;

    if (offset)
    {
        ti_archfile_t * archfile = ti_archfile_get(first, last);
        if (!archfile)
        {
            log_error(
                    "cannot find archive file for event range "
                    TI_EVENT_ID " - "TI_EVENT_ID,
                    first, last);
            goto failed;
        }

        next_pkg = syncevents__pkg(archfile, offset);
        if (!next_pkg)
        {
            log_error(
                    "failed creating package for `%s` using offset: %zd)",
                    archfile->fn, offset);
            goto failed;
        }

        if (ti_req_create(
                req->stream,
                next_pkg,
                TI_PROTO_NODE_REQ_SYNCEPART_TIMEOUT,
                syncevents__push_cb,
                NULL))
        {
            free(next_pkg);
            goto failed;
        }

        goto done;
    }

    rc = ti_syncevents_init(req->stream, last);
    if (rc < 0)
    {
        log_error(
                "failed creating request for stream `%s` and "TI_EVENT_ID,
                ti_stream_name(req->stream),
                last);
        goto failed;
    }

    if (rc > 0)
    {
        if (ti_syncevents_done(req->stream))
            goto failed;
    }

    goto done;

failed:
    ti_stream_stop_watching(req->stream);
done:
    ti_req_destroy(req);
}

static void syncevents__done_cb(ti_req_t * req, ex_enum status)
{
    LOGC("syncevents__done_cb");

    if (status)
        log_error("failed response: `%s` (%s)", ex_str(status), status);

    ti_away_syncer_done(req->stream);
    ti_stream_stop_watching(req->stream);

    ti_req_destroy(req);
}


