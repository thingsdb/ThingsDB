/*
 * ti/fsync.c
 */
#include <ti.h>
#include <qpack.h>
#include <assert.h>
#include <ti/fsync.h>
#include <ti/proto.h>
#include <ti/collection.h>
#include <ti/req.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <util/qpx.h>


#define FSYNC__PART_SIZE 131072UL

typedef enum
{
    FSYNC__ACCESS_FILE
} fsync__file_t;

static int fsync__write_part(
        const char * fn,
        const unsigned char * data,
        size_t size,
        off_t offset,
        ex_t * e);
static void fsync__push_cb(ti_req_t * req, ex_enum status);
static ti_pkg_t * fsync__pkg(
        ti_collection_t * target,
        fsync__file_t ft,
        off_t offset);
static const char * fsync__get_fn(uint64_t target_id, fsync__file_t ft);
static int fsync__part_to_packer(
        qp_packer_t * packer,
        const char * fn,
        off_t offset);

int ti_fsync_start(ti_stream_t * stream)
{
    ti_pkg_t * pkg = fsync__pkg(NULL, FSYNC__ACCESS_FILE, 0);
    if (!pkg)
        return -1;

    if (ti_req_create(
            stream,
            pkg,
            TI_PROTO_NODE_REQ_PUSH_PART_TIMEOUT,
            fsync__push_cb,
            NULL))
    {
        free(pkg);
        return -1;
    }
    return 0;
}

ti_pkg_t * ti_fsync_on_multipart(ti_pkg_t * pkg, ex_t * e)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * resp;
    qp_obj_t qp_target, qp_ft, qp_offset, qp_raw, qp_more;
    qpx_packer_t * packer;
    fsync__file_t ft;
    off_t offset;
    uint64_t target_id;
    const char * fn;

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

    fn = fsync__get_fn(target_id, ft);
    if (!fn)
    {
        ex_set(e, EX_BAD_DATA, "invalid file type: %d", ft);
        return NULL;
    }

    if (fsync__write_part(fn, qp_raw.via.raw, qp_raw.len, offset, e))
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

    resp = qpx_packer_pkg(packer, TI_PROTO_NODE_RES_MULTIPART);
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
    fp = fopen(fn, "a");
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
        ex_set(e, EX_BAD_DATA, "file `%s` is expected to have size %zd (%s)",
                fn,
                offset,
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

static void fsync__push_cb(ti_req_t * req, ex_enum status)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * pkg = req->pkg_res;
    qp_obj_t qp_target, qp_ft, qp_offset;
    uint64_t target_id;
    fsync__file_t ft;
    off_t offset;
    const char * fn;

    if (status)
        goto failed;

    if (pkg->tp != TI_PROTO_NODE_RES_MULTIPART)
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
        log_error("invalid multipart response");
        goto failed;
    }

    target_id = (uint64_t) qp_target.via.int64;
    ft = (fsync__file_t) qp_ft.via.int64;
    offset = (off_t) qp_offset.via.int64;

    fn = fsync__get_fn(target_id, ft);
    if (!fn)
    {
        log_error("invalid file type: %d", ft);
        goto failed;
    }

    if (!offset)
    {
        /* next file or finished */
    }
    else
    {
        /* next part from file */
    }

    goto done;

failed:
    ti_stream_stop_watching(req->stream);
done:
    free(req->pkg_req);
    ti_req_destroy(req);
}

static ti_pkg_t * fsync__pkg(
        ti_collection_t * target,
        fsync__file_t ft,
        off_t offset)
{
    int more;
    const char * fn;
    qpx_packer_t * packer = qpx_packer_create(48 + FSYNC__PART_SIZE, 1);
    if (!packer)
        return NULL;

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, 0);       /* target root */
    (void) qp_add_int(packer, ft);      /* file type */
    (void) qp_add_int(packer, offset);  /* offset in file */

    fn = fsync__get_fn(target ? target->root->id : 0, ft);
    if (!fn)
        goto failed;

    more = fsync__part_to_packer(packer, fn, offset);
    if (more < 0)
        goto failed;

    (void) qp_add_bool(packer, (_Bool) more);
    (void) qp_close_array(packer);

    return qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_MULTIPART);

failed:
    qp_packer_destroy(packer);
    return NULL;
}

static const char * fsync__get_fn(uint64_t target_id, fsync__file_t ft)
{
    if (target_id == 0)
    {
        switch (ft)
        {
        case FSYNC__ACCESS_FILE:
            return ti()->store->access_fn;
        }
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
