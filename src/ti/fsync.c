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

static void fsync__push_cb(ti_req_t * req, ex_enum status);
static ti_pkg_t * fsync__pkg(
        ti_collection_t * target,
        fsync__file_t ft,
        off_t offset);
static const char * fsync__get_fn(ti_collection_t * target, fsync__file_t ft);
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

ti_pkg_t * ti_fsync_on_multipart(ti_pkg_t * pkg)
{

}

static void fsync__push_cb(ti_req_t * req, ex_enum status)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * pkg = req->pkg_res;

    if (status)
    {
        ti_stream_stop_watching(req->stream);
        goto done;
    }

    if (pkg->tp != TI_PROTO_NODE_RES_MULTIPART)
    {
        ti_pkg_log(pkg);
        goto done;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);



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

    (void) qp_add_array(&packer);
    (void) qp_add_int64(packer, 0);        /* target root */
    (void) qp_add_int64(packer, ft);
    (void) qp_add_int64(packer, offset);

    fn = fsync__get_fn(target, ft);
    if (!fn)
        goto failed;

    more = fsync__part_to_packer(packer, fn, offset);
    if (more < 0)
        goto failed;

    if (more)
        (void) qp_add_true(packer);
    else
        (void) qp_add_false(packer);

    (void) qp_close_array(packer);

    return qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_MULTIPART);

failed:
    qp_packer_destroy(packer);
    return NULL;
}

static const char * fsync__get_fn(ti_collection_t * target, fsync__file_t ft)
{
    if (target == NULL)
    {
        switch (ft)
        {
        case  FSYNC__ACCESS_FILE:
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
    int rc;
    size_t sz;
    off_t restsz;
    FILE * fp = fopen(fn, "r");
    unsigned char * buff;
    if (!fp)
        goto fail0;

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
        log_critical("cannot close file `%s`", fn);
        goto fail2;
    }

    rc = qp_add_raw(packer, buff, sz) ? -1 : (size_t) restsz != sz;
    free(buff);
    return rc;

fail2:
    free(buff);
fail1:
    (void) fclose(fp);
fail0:
    return -1;
}
