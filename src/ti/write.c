/*
 * ti/write.c
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/proto.h>
#include <ti/write.h>
#include <util/logger.h>

static void ti__write_cb(uv_write_t * req, int status);

int ti_write(ti_stream_t * stream, ti_pkg_t * pkg, void * data, ti_write_cb cb)
{
    uv_buf_t wrbuf;
    ti_write_t * req = (ti_write_t *) malloc(sizeof(uv_write_t));
    if (!req)
        return -1;

    req->req_.data = req;
    req->stream = stream;
    req->pkg = pkg;
    req->data = data;
    req->cb_ = cb;

    ti_incref(stream);

    wrbuf = uv_buf_init((char *) pkg, sizeof(ti_pkg_t) + pkg->n);
    uv_write(&req->req_, stream->uvstream, &wrbuf, 1, &ti__write_cb);

    return 0;
}

void ti_write_destroy(ti_write_t * req)
{
    ti_stream_drop(req->stream);
    free(req);
}

/*
 * Actually a callback on uv_write.
 */
static void ti__write_cb(uv_write_t * req, int status)
{
    ti_write_t * ti_req = req->data;

    if (status)
        log_error(
                "stream write error (package type: `%s`, error: `%s`)",
                ti_proto_str(ti_req->pkg->tp),
                uv_strerror(status)
        );

    ti_req->cb_(ti_req, status ? EX_WRITE_UV : 0);
}
