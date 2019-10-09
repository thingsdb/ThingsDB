/*
 * ti/syncarchive.c
 */
#include <assert.h>
#include <ti/proto.h>
#include <ti.h>
#include <ti/nodes.h>
#include <ti/syncarchive.h>
#include <ti/syncevents.h>
#include <util/mpack.h>
#include <util/syncpart.h>

static ti_pkg_t * syncarchive__pkg(ti_archfile_t * archfile, off_t offset);
static void syncarchive__push_cb(ti_req_t * req, ex_enum status);
static void syncarchive__done_cb(ti_req_t * req, ex_enum status);
static int syncarchive__init(ti_stream_t * stream, ti_archfile_t * archfile);

/*
 * Returns 1 if no archive file is found for the given `event_id` and 0 if
 * one is found and a request is successfully made. In case of an error, -1
 * will be the return value.
 */
int ti_syncarchive_init(ti_stream_t * stream, uint64_t event_id)
{
    vec_t * archfiles = ti()->archive->archfiles;
    ti_archfile_t * closest = NULL;
    size_t closest_gap = SIZE_MAX;

    for (vec_each(archfiles, ti_archfile_t, archfile))
    {
        if (event_id >= archfile->first && event_id <= archfile->last)
        {
            return syncarchive__init(stream, archfile);
        }
        else if (event_id < archfile->first)
        {
            /*
             * If the given event_id is not found, but an archive with
             * higher id exists, then we have a gap and should synchronize
             * the closest available.
             */
            size_t diff = archfile->first - event_id;
            if (diff < closest_gap)
            {
                closest_gap = diff;
                closest = archfile;
            }
        }
    }

    return closest ? syncarchive__init(stream, closest) : 1;
}

ti_pkg_t * ti_syncarchive_on_part(ti_pkg_t * pkg, ex_t * e)
{
    int rc;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    mp_unp_t up;
    ti_pkg_t * resp;
    mp_obj_t obj, mp_first, mp_last, mp_offset, mp_bin, mp_more;
    off_t offset;
    uint64_t first, last;
    ti_archfile_t * archfile;

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
        mp_next(&up, &mp_first) != MP_U64 ||
        mp_next(&up, &mp_last) != MP_U64 ||
        mp_next(&up, &mp_offset) != MP_I64 ||
        mp_next(&up, &mp_bin) != MP_BIN ||
        mp_next(&up, &mp_more) != MP_BOOL)
    {
        ex_set(e, EX_BAD_DATA, "invalid multipart request");
        return NULL;
    }

    first = mp_first.via.u64;
    last = mp_last.via.u64;
    offset = (off_t) mp_offset.via.i64;

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
        if (!archfile)
        {
            ex_set_mem(e);
            return NULL;
        }

        (void) vec_push(&archive->archfiles, archfile);
    }

    rc = syncpart_write(
            archfile->fn,
            mp_bin.via.bin.data,
            mp_bin.via.bin.n,
            offset,
            e);

    if (rc)
        return NULL;

    if (mp_more.via.bool_)
    {
        offset += mp_bin.via.bin.n;
    }
    else
    {
        first = 0;
        last += 1;
        offset = 0;
    }

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
    {
        ex_set_mem(e);
        return NULL;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 3);
    msgpack_pack_uint64(&pk, first);
    msgpack_pack_uint64(&pk, last);
    msgpack_pack_int64(&pk, offset);

    resp = (ti_pkg_t *) buffer.data;
    pkg_init(resp, pkg->id, TI_PROTO_NODE_RES_SYNCAPART, buffer.size);

    return resp;
}


static ti_pkg_t * syncarchive__pkg(ti_archfile_t * archfile, off_t offset)
{
    int more;
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, 64 + SYNCPART_SIZE, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 5);
    msgpack_pack_uint64(&pk, archfile->first);
    msgpack_pack_uint64(&pk, archfile->last);
    msgpack_pack_int64(&pk, offset);

    more = syncpart_to_pk(&pk, archfile->fn, offset);
    if (more < 0)
        goto failed;

    mp_pack_bool(&pk, (_Bool) more);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_REQ_SYNCAPART, buffer.size);

    return pkg;

failed:
    msgpack_sbuffer_destroy(&buffer);
    return NULL;
}

static void syncarchive__push_cb(ti_req_t * req, ex_enum status)
{
    mp_unp_t up;
    ti_pkg_t * pkg = req->pkg_res;
    ti_pkg_t * next_pkg;
    mp_obj_t obj, mp_first, mp_last, mp_offset;
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

    if (pkg->tp != TI_PROTO_NODE_RES_SYNCAPART)
    {
        ti_pkg_log(pkg);
        goto failed;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 3 ||
        mp_next(&up, &mp_first) != MP_U64 ||
        mp_next(&up, &mp_last) != MP_U64 ||
        mp_next(&up, &mp_offset) != MP_I64)
    {
        log_error("invalid `%s`", ti_proto_str(pkg->tp));
        goto failed;
    }

    first = mp_first.via.u64;
    last = mp_last.via.u64;
    offset = (off_t) mp_offset.via.i64;

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

        next_pkg = syncarchive__pkg(archfile, offset);
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
                TI_PROTO_NODE_REQ_SYNCAPART_TIMEOUT,
                syncarchive__push_cb,
                NULL))
        {
            free(next_pkg);
            goto failed;
        }

        goto done;
    }

    rc = ti_syncarchive_init(req->stream, last);

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
        next_pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_SYNCADONE, NULL, 0);

        if (!next_pkg)
            goto failed;

        if (ti_req_create(
                req->stream,
                next_pkg,
                TI_PROTO_NODE_REQ_SYNCADONE_TIMEOUT,
                syncarchive__done_cb,
                NULL))
        {
            free(next_pkg);
            goto failed;
        }
    }

    goto done;

failed:
    ti_stream_stop_watching(req->stream);
done:
    ti_req_destroy(req);
}

static void syncarchive__done_cb(ti_req_t * req, ex_enum status)
{
    int rc;
    uint64_t next_event_id = ti()->node->sevid + 1;

    if (status)
        log_error("failed response: `%s` (%s)", ex_str(status), status);

    rc = ti_syncevents_init(req->stream, next_event_id);

    if (rc > 0)
    {
        rc = ti_syncevents_done(req->stream);
    }

    if (rc < 0)
    {
        log_error(
                "failed creating request for stream `%s` and "TI_EVENT_ID,
                ti_stream_name(req->stream),
                next_event_id);
    }

    ti_req_destroy(req);
}

static int syncarchive__init(ti_stream_t * stream, ti_archfile_t * archfile)
{
    ti_pkg_t * pkg = syncarchive__pkg(archfile, 0 /* offset */);
    if (!pkg)
        return -1;

    if (ti_req_create(
            stream,
            pkg,
            TI_PROTO_NODE_REQ_SYNCAPART_TIMEOUT,
            syncarchive__push_cb,
            NULL))
    {
        free(pkg);
        return -1;
    }

    log_debug(
            "synchronizing archive ("TI_EVENT_ID"-"TI_EVENT_ID") to `%s`",
            archfile->first,
            archfile->last,
            ti_stream_name(stream));

    return 0;
}
