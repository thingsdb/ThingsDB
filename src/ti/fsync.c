/*
 * ti/fsync.c
 */
#include <ti.h>
#include <qpack.h>
#include <assert.h>
#include <ti/fsync.h>
#include <ti/proto.h>
#include <ti/collection.h>
#include <ti/store/collection.h>
#include <ti/store.h>
#include <ti/req.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <util/qpx.h>


#define FSYNC__PART_SIZE 131072UL

typedef enum
{
    /* root files */
    FSYNC__USERS_FILE,
    FSYNC__ACCESS_FILE,
    FSYNC__NAMES_FILE,
    FSYNC__COLLECTIONS_FILE,
    FSYNC__ID_STAT_FILE,
    /* collection files */
    FSYNC__COLLECTION_DAT_FILE,
    FSYNC__COLLECTION_ACCESS_FILE,
    FSYNC__COLLECTION_THINGS_FILE,
    FSYNC__COLLECTION_PROPS_FILE,
    FSYNC__COLLECTION_END,
} fsync__file_t;

static int fsync__write_part(
        const char * fn,
        const unsigned char * data,
        size_t size,
        off_t offset,
        ex_t * e);
static void fsync__push_cb(ti_req_t * req, ex_enum status);
static ti_pkg_t * fsync__pkg(
        uint64_t target_id,
        fsync__file_t ft,
        off_t offset);
static char * fsync__get_fn(uint64_t target_id, fsync__file_t ft);
static int fsync__part_to_packer(
        qp_packer_t * packer,
        const char * fn,
        off_t offset);

int ti_fsync_start(ti_stream_t * stream)
{
    ti_pkg_t * pkg = fsync__pkg(0, FSYNC__USERS_FILE, 0);
    if (!pkg)
        return -1;

    if (ti_req_create(
            stream,
            pkg,
            TI_PROTO_NODE_REQ_FSYNCPART_TIMEOUT,
            fsync__push_cb,
            NULL))
    {
        free(pkg);
        return -1;
    }
    return 0;
}

ti_pkg_t * ti_fsync_on_part(ti_pkg_t * pkg, ex_t * e)
{
    int rc;
    qp_unpacker_t unpacker;
    ti_pkg_t * resp;
    qp_obj_t qp_target, qp_ft, qp_offset, qp_raw, qp_more;
    qpx_packer_t * packer;
    fsync__file_t ft;
    off_t offset;
    uint64_t target_id;
    char * fn;

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_target)) ||
        !qp_is_int(qp_next(&unpacker, &qp_ft)) ||
        !qp_is_int(qp_next(&unpacker, &qp_offset)) ||
        !qp_is_raw(qp_next(&unpacker, &qp_raw)) ||
        !qp_is_bool(qp_next(&unpacker, &qp_more)))
    {
        ex_set(e, EX_BAD_DATA, "invalid multipart request");
        return NULL;
    }

    target_id = (uint64_t) qp_target.via.int64;
    ft = (fsync__file_t) qp_ft.via.int64;
    offset = (off_t) qp_offset.via.int64;

    if (ft == FSYNC__COLLECTION_DAT_FILE)
    {
        int rc;
        char * path = ti()->store->store_path;
        path = ti_store_collection_get_path(path, target_id);
        rc = mkdir(path, 0700);
        if (rc)
        {
            log_error("cannot create directory `%s` (%s)",
                    path,
                    strerror(errno));
        }
        free(path);
    }

    fn = fsync__get_fn(target_id, ft);
    if (!fn)
    {
        ex_set(e, EX_BAD_DATA, "invalid file type %d for "TI_COLLECTION_ID,
                ft, target_id);
        return NULL;
    }

    rc = fsync__write_part(fn, qp_raw.via.raw, qp_raw.len, offset, e);
    free(fn);
    if (rc)
        return NULL;

    packer = qpx_packer_create(48 , 1);
    if (!packer)
    {
        ex_set_alloc(e);
        return NULL;
    }

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, qp_target.via.int64);
    (void) qp_add_int(packer, qp_ft.via.int64);
    (void) qp_add_int(packer, qp_more.tp == QP_TRUE ? offset + qp_raw.len : 0);
    (void) qp_close_array(packer);

    resp = qpx_packer_pkg(packer, TI_PROTO_NODE_RES_FSYNCPART);
    resp->id = pkg->id;

    return resp;
}

static int fsync__write_part(
        const char * fn,
        const unsigned char * data,
        size_t size,
        off_t offset,
        ex_t * e)
{
    off_t sz;
    FILE * fp;
    fp = fopen(fn, offset ? "ab" : "wb");
    if (!fp)
    {
        /* lock is required for use of strerror */
        uv_mutex_lock(&Logger.lock);
        ex_set(e, EX_INTERNAL,
                "cannot open file `%s` (%s)",
                fn, strerror(errno));
        uv_mutex_unlock(&Logger.lock);
        return e->nr;
    }
    sz = ftello(fp);
    if (sz != offset)
    {
        uv_mutex_lock(&Logger.lock);
        ex_set(e, EX_BAD_DATA,
                "file `%s` is expected to have size %zd (got: %zd, %s)",
                fn,
                offset,
                sz,
                sz == -1 ? strerror(errno) : "file size is different");
        uv_mutex_unlock(&Logger.lock);
        goto done;
    }

    if (fwrite(data, sizeof(char), size, fp) != size)
    {
        ex_set(e, EX_INTERNAL, "error writing %zu bytes to file `%s`",
                size, fn);
    }

done:
    if (fclose(fp) && !e->nr)
    {
        uv_mutex_lock(&Logger.lock);
        ex_set(e, EX_INTERNAL, "cannot close file `%s` (%s)",
                fn, strerror(errno));
        uv_mutex_unlock(&Logger.lock);
    }

    return e->nr;
}

static bool fsync__next_file(uint64_t * target_id, fsync__file_t * ft)
{
    (*ft)++;
    if (*ft == FSYNC__COLLECTION_END)
        *ft = FSYNC__COLLECTION_DAT_FILE;

    if (*ft == FSYNC__COLLECTION_DAT_FILE)
    {
        size_t i = 0;
        ti_collection_t * collection;
        vec_t * collections = ti()->collections->vec;

        if (*target_id)
            for (vec_each(collections, ti_collection_t, collection))
                if (++i && collection->root->id == *target_id)
                    break;
        collection = vec_get_or_null(collections, i);
        if (!collection)
            return false;       /* finished, no more files to sync */
        *target_id = collection->root->id;
    }
    return true;
}

static void fsync__done_cb(ti_req_t * req, ex_enum status)
{
    if (status)
        log_error("failed response on fsync done");

    LOGC("FSYNC DONE CB");

    ti_away_syncer_done(req->stream);
    ti_stream_stop_watching(req->stream);

    free(req->pkg_req);
    ti_req_destroy(req);
}

static void fsync__push_cb(ti_req_t * req, ex_enum status)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * pkg = req->pkg_res;
    ti_pkg_t * next_pkg;
    qp_obj_t qp_target, qp_ft, qp_offset;
    uint64_t target_id;
    fsync__file_t ft;
    off_t offset;

    LOGC("ON CALLBACK");

    if (status)
        goto failed;

    if (!req->stream)
    {
        log_error("connection to stream lost while synchronizing");
        goto failed;
    }

    if (pkg->tp != TI_PROTO_NODE_RES_FSYNCPART)
    {
        ti_pkg_log(pkg);
        goto failed;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_target)) ||
        !qp_is_int(qp_next(&unpacker, &qp_ft)) ||
        !qp_is_int(qp_next(&unpacker, &qp_offset)))
    {
        log_error("invalid `%s`", ti_proto_str(pkg->tp));
        goto failed;
    }

    target_id = (uint64_t) qp_target.via.int64;
    ft = (fsync__file_t) qp_ft.via.int64;
    offset = (off_t) qp_offset.via.int64;

    if (!offset && !fsync__next_file(&target_id, &ft))
    {
        LOGC("FINISED WITH ALL FILES");
        next_pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_FSYNCDONE, NULL, 0);
        if (!next_pkg)
            goto failed;

        if (ti_req_create(
                req->stream,
                next_pkg,
                TI_PROTO_NODE_REQ_FSYNCDONE_TIMEOUT,
                fsync__done_cb,
                NULL))
        {
            free(next_pkg);
            goto failed;
        }
        goto done;
    }

    next_pkg = fsync__pkg(target_id, ft, offset);
    if (!next_pkg)
    {
        log_error(
                "failed creating package "
                "(target: %"PRIu64" file type: %d, offset: %zd)",
                target_id, ft, offset);
        goto failed;
    }

    if (ti_req_create(
            req->stream,
            next_pkg,
            TI_PROTO_NODE_REQ_FSYNCPART_TIMEOUT,
            fsync__push_cb,
            NULL))
    {
        free(next_pkg);
        goto failed;
    }

    goto done;

failed:
    ti_stream_stop_watching(req->stream);
done:
    free(req->pkg_req);
    ti_req_destroy(req);
}

static ti_pkg_t * fsync__pkg(
        uint64_t target_id,
        fsync__file_t ft,
        off_t offset)
{
    int more;
    char * fn;
    qpx_packer_t * packer = qpx_packer_create(48 + FSYNC__PART_SIZE, 1);
    if (!packer)
        return NULL;

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, target_id);   /* target root */
    (void) qp_add_int(packer, ft);          /* file type */
    (void) qp_add_int(packer, offset);      /* offset in file */

    fn = fsync__get_fn(target_id, ft);
    if (!fn)
        goto failed;

    more = fsync__part_to_packer(packer, fn, offset);
    free(fn);
    if (more < 0)
        goto failed;

    (void) qp_add_bool(packer, (_Bool) more);
    (void) qp_close_array(packer);

    return qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_FSYNCPART);

failed:
    qp_packer_destroy(packer);
    return NULL;
}

static char * fsync__get_fn(uint64_t target_id, fsync__file_t ft)
{
    char * path = ti()->store->store_path;

    switch (ft)
    {
    case FSYNC__USERS_FILE:
        return strdup(ti()->store->users_fn);
    case FSYNC__ACCESS_FILE:
        return strdup(ti()->store->access_fn);
    case FSYNC__NAMES_FILE:
        return strdup(ti()->store->names_fn);
    case FSYNC__COLLECTIONS_FILE:
        return strdup(ti()->store->collections_fn);
    case FSYNC__ID_STAT_FILE:
        return strdup(ti()->store->id_stat_fn);
    case FSYNC__COLLECTION_DAT_FILE:
        return ti_store_collection_dat_fn(path, target_id);
    case FSYNC__COLLECTION_ACCESS_FILE:
        return ti_store_collection_access_fn(path, target_id);
    case FSYNC__COLLECTION_THINGS_FILE:
        return ti_store_collection_things_fn(path, target_id);
    case FSYNC__COLLECTION_PROPS_FILE:
        return ti_store_collection_props_fn(path, target_id);
    case FSYNC__COLLECTION_END:
        break;
    }

    return NULL;
}

/*
 * Returns 0 if the file is complete, 1 if more data is available and -1 on
 * error.
 */
static int fsync__part_to_packer(
        qp_packer_t * packer,
        const char * fn,
        off_t offset)
{
    int more;
    size_t sz;
    off_t restsz;
    unsigned char * buff;
    FILE * fp = fopen(fn, "r");
    if (!fp)
    {
        log_critical("cannot open file `%s` (%s)", fn, strerror(errno));
        goto fail0;
    }

    if (fseeko(fp, 0, SEEK_END) == -1 || (restsz = ftello(fp)) == -1)
    {
        log_critical("error seeking file `%s` (%s)", fn, strerror(errno));
        goto fail1;
    }

    if (offset > restsz)
    {
        log_critical("got an illegal offset for file `%s` (%zd)", fn, offset);
        goto fail1;
    }

    restsz -= offset;

    sz = (size_t) restsz > FSYNC__PART_SIZE
            ? FSYNC__PART_SIZE
            : (size_t) restsz;

    buff = malloc(sz);
    if (!buff)
        goto fail1;

    if (fseeko(fp, offset, SEEK_SET) == -1)
    {
        log_critical("error seeking file `%s` (%s)", fn, strerror(errno));
        goto fail2;
    }

    if (fread(buff, sizeof(char), sz, fp) != sz)
    {
        log_critical("cannot read %zu bytes from file `%s`", sz, fn);
        goto fail2;
    }

    if (fclose(fp))
    {
        log_critical("cannot close file `%s` (%s)", fn, strerror(errno));
        goto fail2;
    }

    more = qp_add_raw(packer, buff, sz) ? -1 : (size_t) restsz != sz;
    free(buff);
    return more;

fail2:
    free(buff);
fail1:
    (void) fclose(fp);
fail0:
    return -1;
}
