/*
 * ti/fsync.c
 */
#include <assert.h>
#include <errno.h>
#include <qpack.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/collection.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/store/storecollection.h>
#include <ti/store.h>
#include <ti/syncarchive.h>
#include <ti/syncevents.h>
#include <ti/syncfull.h>
#include <util/qpx.h>
#include <util/fx.h>
#include <util/syncpart.h>

typedef enum
{
    /* root files */
    SYNCFULL__USERS_FILE,
    SYNCFULL__ACCESS_NODE_FILE,
    SYNCFULL__ACCESS_THINGSDB_FILE,
    SYNCFULL__PROCEDURES_FILE,
    SYNCFULL__NAMES_FILE,
    SYNCFULL__COLLECTIONS_FILE,
    SYNCFULL__ID_STAT_FILE,
    /* collection files */
    SYNCFULL__COLLECTION_DAT_FILE,
    SYNCFULL__COLLECTION_TYPES_FILE,
    SYNCFULL__COLLECTION_ACCESS_FILE,
    SYNCFULL__COLLECTION_PROCEDURES_FILE,
    SYNCFULL__COLLECTION_THINGS_FILE,
    SYNCFULL__COLLECTION_PROPS_FILE,
    /* end */
    SYNCFULL__COLLECTION_END,
} syncfull__file_t;

static char * syncfull__get_fn(uint64_t scope_id, syncfull__file_t ft)
{
    char * path = ti()->store->store_path;

    switch (ft)
    {
    case SYNCFULL__USERS_FILE:
        return strdup(ti()->store->users_fn);
    case SYNCFULL__ACCESS_NODE_FILE:
        return strdup(ti()->store->access_node_fn);
    case SYNCFULL__ACCESS_THINGSDB_FILE:
        return strdup(ti()->store->access_thingsdb_fn);
    case SYNCFULL__PROCEDURES_FILE:
        return strdup(ti()->store->procedures_fn);
    case SYNCFULL__NAMES_FILE:
        return strdup(ti()->store->names_fn);
    case SYNCFULL__COLLECTIONS_FILE:
        return strdup(ti()->store->collections_fn);
    case SYNCFULL__ID_STAT_FILE:
        return strdup(ti()->store->id_stat_fn);
    case SYNCFULL__COLLECTION_DAT_FILE:
        return ti_store_collection_dat_fn(path, scope_id);
    case SYNCFULL__COLLECTION_TYPES_FILE:
        return ti_store_collection_types_fn(path, scope_id);
    case SYNCFULL__COLLECTION_ACCESS_FILE:
        return ti_store_collection_access_fn(path, scope_id);
    case SYNCFULL__COLLECTION_PROCEDURES_FILE:
        return ti_store_collection_procedures_fn(path, scope_id);
    case SYNCFULL__COLLECTION_THINGS_FILE:
        return ti_store_collection_things_fn(path, scope_id);
    case SYNCFULL__COLLECTION_PROPS_FILE:
        return ti_store_collection_props_fn(path, scope_id);
    case SYNCFULL__COLLECTION_END:
        break;
    }

    return NULL;
}

static _Bool syncfull__next_file(uint64_t * scope_id, syncfull__file_t * ft)
{
    (*ft)++;
    if (*ft == SYNCFULL__COLLECTION_END)
        *ft = SYNCFULL__COLLECTION_DAT_FILE;

    if (*ft == SYNCFULL__COLLECTION_DAT_FILE)
    {
        size_t i = 0;
        uint64_t * collection_id;
        vec_t * collection_ids = ti()->store->collection_ids;
        char * path = ti()->store->store_path;

        if (*scope_id)
            for (vec_each(collection_ids, uint64_t, collection_id))
                if (++i && *collection_id == *scope_id)
                    break;

        while (1)
        {
            collection_id = vec_get_or_null(collection_ids, i);
            if (!collection_id)
                return false;       /* finished, no more files to sync */

            *scope_id = *collection_id;

            if (ti_store_collection_is_stored(path, *scope_id))
                break;

            log_error("path is missing for: "TI_COLLECTION_ID, *scope_id);
            ++i;
        }
    }
    return true;
}

static void syncfull__done_cb(ti_req_t * req, ex_enum status)
{
    int rc;
    uint64_t next_event_id = ti()->store->last_stored_event_id + 1;

    if (status)
        log_error("failed response: `%s` (%s)", ex_str(status), status);

    rc = ti_syncarchive_init(req->stream, next_event_id);

    if (rc > 0)
    {
        rc = ti_syncevents_init(req->stream, next_event_id);

        if (rc > 0)
        {
            rc = ti_syncevents_done(req->stream);
        }
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

static ti_pkg_t * syncfull__pkg(
        uint64_t scope_id,
        syncfull__file_t ft,
        off_t offset)
{
    int more;
    char * fn;
    qpx_packer_t * packer = qpx_packer_create(48 + SYNCPART_SIZE, 1);
    if (!packer)
        return NULL;

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, scope_id);    /* scope */
    (void) qp_add_int(packer, ft);          /* file type */
    (void) qp_add_int(packer, offset);      /* offset in file */

    fn = syncfull__get_fn(scope_id, ft);
    if (!fn)
        goto failed;

    more = syncpart_to_pk(packer, fn, offset);
    free(fn);
    if (more < 0)
        goto failed;

    (void) qp_add_bool(packer, (_Bool) more);
    (void) qp_close_array(packer);

    return qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_SYNCFPART);

failed:
    qp_packer_destroy(packer);
    return NULL;
}

static void syncfull__push_cb(ti_req_t * req, ex_enum status)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * pkg = req->pkg_res;
    ti_pkg_t * next_pkg;
    qp_obj_t qp_scope, qp_ft, qp_offset;
    uint64_t scope_id;
    syncfull__file_t ft;
    off_t offset;

    if (status)
        goto failed;

    if (!req->stream)
    {
        log_error("connection to stream lost while synchronizing");
        goto failed;
    }

    if (pkg->tp != TI_PROTO_NODE_RES_SYNCFPART)
    {
        ti_pkg_log(pkg);
        goto failed;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_scope)) ||
        !qp_is_int(qp_next(&unpacker, &qp_ft)) ||
        !qp_is_int(qp_next(&unpacker, &qp_offset)))
    {
        log_error("invalid `%s`", ti_proto_str(pkg->tp));
        goto failed;
    }

    scope_id = (uint64_t) qp_scope.via.int64;
    ft = (syncfull__file_t) qp_ft.via.int64;
    offset = (off_t) qp_offset.via.int64;

    if (!offset && !syncfull__next_file(&scope_id, &ft))
    {
        next_pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_SYNCFDONE, NULL, 0);
        if (!next_pkg)
            goto failed;

        if (ti_req_create(
                req->stream,
                next_pkg,
                TI_PROTO_NODE_REQ_SYNCFDONE_TIMEOUT,
                syncfull__done_cb,
                NULL))
        {
            free(next_pkg);
            goto failed;
        }
        goto done;
    }

    next_pkg = syncfull__pkg(scope_id, ft, offset);
    if (!next_pkg)
    {
        log_error(
                "failed creating package "
                "(scope id: %"PRIu64" file type: %d, offset: %zd)",
                scope_id, ft, offset);
        goto failed;
    }

    if (ti_req_create(
            req->stream,
            next_pkg,
            TI_PROTO_NODE_REQ_SYNCFPART_TIMEOUT,
            syncfull__push_cb,
            NULL))
    {
        free(next_pkg);
        goto failed;
    }

    goto done;

failed:
    ti_stream_stop_watching(req->stream);
done:
    ti_req_destroy(req);
}

int ti_syncfull_start(ti_stream_t * stream)
{
    ti_pkg_t * pkg = syncfull__pkg(0, SYNCFULL__USERS_FILE, 0);
    if (!pkg)
        return -1;

    if (ti_req_create(
            stream,
            pkg,
            TI_PROTO_NODE_REQ_SYNCFPART_TIMEOUT,
            syncfull__push_cb,
            NULL))
    {
        free(pkg);
        return -1;
    }
    return 0;
}

ti_pkg_t * ti_syncfull_on_part(ti_pkg_t * pkg, ex_t * e)
{
    int rc;
    qp_unpacker_t unpacker;
    ti_pkg_t * resp;
    qp_obj_t qp_scope, qp_ft, qp_offset, qp_raw, qp_more;
    qpx_packer_t * packer;
    syncfull__file_t ft;
    off_t offset;
    uint64_t scope_id;
    char * fn;

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_scope)) ||
        !qp_is_int(qp_next(&unpacker, &qp_ft)) ||
        !qp_is_int(qp_next(&unpacker, &qp_offset)) ||
        !qp_is_raw(qp_next(&unpacker, &qp_raw)) ||
        !qp_is_bool(qp_next(&unpacker, &qp_more)))
    {
        ex_set(e, EX_BAD_DATA, "invalid multipart request");
        return NULL;
    }

    scope_id = (uint64_t) qp_scope.via.int64;
    ft = (syncfull__file_t) qp_ft.via.int64;
    offset = (off_t) qp_offset.via.int64;

    if (ft == SYNCFULL__COLLECTION_DAT_FILE)
    {
        int rc;
        char * path = ti()->store->store_path;
        path = ti_store_collection_get_path(path, scope_id);
        if (fx_is_dir(path))
        {
            log_debug("delete existing path: `%s`", path);
            if (fx_rmdir(path))
            {
                log_warning("cannot remove directory: `%s`", path);
            }
        }
        rc = mkdir(path, 0700);
        if (rc)
        {
            log_error("cannot create directory `%s` (%s)",
                    path,
                    strerror(errno));
        }
        free(path);
    }

    fn = syncfull__get_fn(scope_id, ft);
    if (!fn)
    {
        ex_set(e, EX_BAD_DATA, "invalid file type %d for "TI_COLLECTION_ID,
                ft, scope_id);
        return NULL;
    }

    rc = syncpart_write(fn, qp_raw.via.raw, qp_raw.len, offset, e);
    free(fn);
    if (rc)
        return NULL;

    packer = qpx_packer_create(48 , 1);
    if (!packer)
    {
        ex_set_mem(e);
        return NULL;
    }

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, qp_scope.via.int64);
    (void) qp_add_int(packer, qp_ft.via.int64);
    (void) qp_add_int(packer, qp_is_true(qp_more.tp) ? offset + qp_raw.len : 0);
    (void) qp_close_array(packer);

    resp = qpx_packer_pkg(packer, TI_PROTO_NODE_RES_SYNCFPART);
    resp->id = pkg->id;

    return resp;
}


