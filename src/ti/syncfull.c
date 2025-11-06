/*
 * ti/fsync.c
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/collection.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/store.h>
#include <ti/store/storecollection.h>
#include <ti/syncarchive.h>
#include <ti/syncevents.h>
#include <ti/syncfull.h>
#include <util/fx.h>
#include <util/mpack.h>
#include <util/syncpart.h>

typedef enum
{
    /* root files */
    SYNCFULL__USERS_FILE,
    SYNCFULL__ACCESS_NODE_FILE,
    SYNCFULL__ACCESS_THINGSDB_FILE,
    SYNCFULL__PROCEDURES_FILE,
    SYNCFULL__TASKS_FILE,
    SYNCFULL__NAMES_FILE,
    SYNCFULL__COLLECTIONS_FILE,
    SYNCFULL__MODULES_FILE,
    SYNCFULL__ID_STAT_FILE,
    SYNCFULL__COMMITS_FILE,
    /* collection files */
    SYNCFULL__COLLECTION_DAT_FILE,
    SYNCFULL__COLLECTION_TYPES_FILE,
    SYNCFULL__COLLECTION_ENUMS_FILE,
    SYNCFULL__COLLECTION_GCPROPS_FILE,
    SYNCFULL__COLLECTION_GCTHINGS_FILE,
    SYNCFULL__COLLECTION_ACCESS_FILE,
    SYNCFULL__COLLECTION_PROCEDURES_FILE,
    SYNCFULL__COLLECTION_TASKS_FILE,
    SYNCFULL__COLLECTION_THINGS_FILE,
    SYNCFULL__COLLECTION_PROPS_FILE,
    SYNCFULL__COLLECTION_NAMED_ROOMS_FILE,
    SYNCFULL__COLLECTION_COMMITS_FILE,
    /* end */
    SYNCFULL__COLLECTION_END,
} syncfull__file_t;

static char * syncfull__get_fn(uint64_t scope_id, syncfull__file_t ft)
{
    char * path = ti.store->store_path;

    switch (ft)
    {
    case SYNCFULL__USERS_FILE:
        return strdup(ti.store->users_fn);
    case SYNCFULL__ACCESS_NODE_FILE:
        return strdup(ti.store->access_node_fn);
    case SYNCFULL__ACCESS_THINGSDB_FILE:
        return strdup(ti.store->access_thingsdb_fn);
    case SYNCFULL__PROCEDURES_FILE:
        return strdup(ti.store->procedures_fn);
    case SYNCFULL__TASKS_FILE:
        return strdup(ti.store->tasks_fn);
    case SYNCFULL__NAMES_FILE:
        return strdup(ti.store->names_fn);
    case SYNCFULL__COLLECTIONS_FILE:
        return strdup(ti.store->collections_fn);
    case SYNCFULL__MODULES_FILE:
        return strdup(ti.store->modules_fn);
    case SYNCFULL__ID_STAT_FILE:
        return strdup(ti.store->id_stat_fn);
    case SYNCFULL__COMMITS_FILE:
        return strdup(ti.store->commits_fn);
    case SYNCFULL__COLLECTION_DAT_FILE:
        return ti_store_collection_dat_fn(path, scope_id);
    case SYNCFULL__COLLECTION_TYPES_FILE:
        return ti_store_collection_types_fn(path, scope_id);
    case SYNCFULL__COLLECTION_ENUMS_FILE:
        return ti_store_collection_enums_fn(path, scope_id);
    case SYNCFULL__COLLECTION_GCPROPS_FILE:
        return ti_store_collection_gcprops_fn(path, scope_id);
    case SYNCFULL__COLLECTION_GCTHINGS_FILE:
        return ti_store_collection_gcthings_fn(path, scope_id);
    case SYNCFULL__COLLECTION_ACCESS_FILE:
        return ti_store_collection_access_fn(path, scope_id);
    case SYNCFULL__COLLECTION_PROCEDURES_FILE:
        return ti_store_collection_procedures_fn(path, scope_id);
    case SYNCFULL__COLLECTION_TASKS_FILE:
        return ti_store_collection_tasks_fn(path, scope_id);
    case SYNCFULL__COLLECTION_THINGS_FILE:
        return ti_store_collection_things_fn(path, scope_id);
    case SYNCFULL__COLLECTION_PROPS_FILE:
        return ti_store_collection_props_fn(path, scope_id);
    case SYNCFULL__COLLECTION_NAMED_ROOMS_FILE:
        return ti_store_collection_named_rooms_fn(path, scope_id);
    case SYNCFULL__COLLECTION_COMMITS_FILE:
        return ti_store_collection_commits_fn(path, scope_id);

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
        vec_t * collection_ids = ti.store->collection_ids;
        char * path = ti.store->store_path;

        if (*scope_id)
            for (vec_each(collection_ids, uint64_t, collection_id))
                if (++i && *collection_id == *scope_id)
                    break;

        while (1)
        {
            collection_id = vec_get(collection_ids, i);
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
    uint64_t next_change_id = ti.store->last_stored_change_id + 1;

    if (status)
        log_error("failed response: `%s` (%d)", ex_str(status), status);

    rc = ti_syncarchive_init(req->stream, next_change_id);

    if (rc > 0)
    {
        rc = ti_syncevents_init(req->stream, next_change_id);

        if (rc > 0)
        {
            rc = ti_syncevents_done(req->stream);
        }
    }

    if (rc < 0)
    {
        log_error(
                "failed creating request for stream `%s` and "TI_CHANGE_ID,
                ti_stream_name(req->stream),
                next_change_id);
    }

    ti_req_destroy(req);
}

static ti_pkg_t * syncfull__pkg(
        uint64_t scope_id,
        syncfull__file_t ft,
        off_t offset)
{
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    int more;
    char * fn;

    if (mp_sbuffer_alloc_init(&buffer, 64 + SYNCPART_SIZE, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 5);

    msgpack_pack_uint64(&pk, scope_id);    /* scope */
    msgpack_pack_uint8(&pk, ft);           /* file type */
    msgpack_pack_fix_int64(&pk, offset);   /* offset in file */

    fn = syncfull__get_fn(scope_id, ft);
    if (!fn)
        goto failed;

    more = syncpart_to_pk(&pk, fn, offset);
    free(fn);
    if (more < 0)
        goto failed;

    mp_pack_bool(&pk, (_Bool) more);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_REQ_SYNCFPART, buffer.size);

    return pkg;

failed:
    msgpack_sbuffer_destroy(&buffer);
    return NULL;
}

static void syncfull__push_cb(ti_req_t * req, ex_enum status)
{
    mp_unp_t up;
    ti_pkg_t * pkg = req->pkg_res;
    ti_pkg_t * next_pkg;
    mp_obj_t obj, mp_scope, mp_ft, mp_offset;
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

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 3 ||
        mp_next(&up, &mp_scope) != MP_U64 ||
        mp_next(&up, &mp_ft) != MP_U64 ||
        mp_next(&up, &mp_offset) != MP_I64)
    {
        log_error("invalid `%s`", ti_proto_str(pkg->tp));
        goto failed;
    }

    scope_id = mp_scope.via.u64;
    ft = (syncfull__file_t) mp_ft.via.u64;
    offset = (off_t) mp_offset.via.i64;

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
    ti_stream_stop_listeners(req->stream);
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
    mp_unp_t up;
    ti_pkg_t * resp;
    mp_obj_t obj, mp_scope, mp_ft, mp_offset, mp_bin, mp_more;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    syncfull__file_t ft;
    off_t offset;
    uint64_t scope_id;
    char * fn;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
        mp_next(&up, &mp_scope) != MP_U64 ||
        mp_next(&up, &mp_ft) != MP_U64 ||
        mp_next(&up, &mp_offset) != MP_I64 ||
        mp_next(&up, &mp_bin) != MP_BIN ||
        mp_next(&up, &mp_more) != MP_BOOL)
    {
        ex_set(e, EX_BAD_DATA, "invalid multipart request (full sync)");
        return NULL;
    }

    scope_id = mp_scope.via.u64;
    ft = (syncfull__file_t) mp_ft.via.u64;
    offset = (off_t) mp_offset.via.i64;

    if (ft == SYNCFULL__COLLECTION_DAT_FILE)
    {
        int rc;
        char * path = ti.store->store_path;
        path = ti_store_collection_get_path(path, scope_id);
        if (fx_is_dir(path))
        {
            log_debug("delete existing path: `%s`", path);
            if (fx_rmdir(path))
                log_warning("cannot remove directory: `%s`", path);
        }
        rc = mkdir(path, FX_DEFAULT_DIR_ACCESS);
        if (rc)
            log_errno_file("cannot create directory", errno, path);
        free(path);
    }

    fn = syncfull__get_fn(scope_id, ft);
    if (!fn)
    {
        ex_set(e, EX_BAD_DATA, "invalid file type %d for "TI_COLLECTION_ID,
                ft, scope_id);
        return NULL;
    }

    rc = syncpart_write(fn, mp_bin.via.bin.data, mp_bin.via.bin.n, offset, e);
    free(fn);
    if (rc)
        return NULL;

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
    {
        ex_set_mem(e);
        return NULL;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 3);
    msgpack_pack_uint64(&pk, mp_scope.via.u64);
    msgpack_pack_uint64(&pk, mp_ft.via.u64);
    msgpack_pack_fix_int64(&pk, mp_more.via.bool_?offset+mp_bin.via.bin.n:0);

    resp = (ti_pkg_t *) buffer.data;
    pkg_init(resp, pkg->id, TI_PROTO_NODE_RES_SYNCFPART, buffer.size);

    return resp;
}


