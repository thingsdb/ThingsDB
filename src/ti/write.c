/*
 * write.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <ti/ref.h>
#include <ti/write.h>

static void ti__write_cb(uv_write_t * req, int status);

int ti_write(ti_stream_t * stream, ti_pkg_t * pkg, void * data, ti_write_cb cb)
{
    ti_write_t * req = (ti_write_t *) malloc(sizeof(uv_write_t));
    if (!req)
        return -1;

    req->req_.data = req;
    req->stream = ti_stream_grab(stream);
    req->pkg = pkg;
    req->data = data;
    req->cb_ = cb;

    uv_buf_t wrbuf = uv_buf_init((char *) pkg, sizeof(ti_pkg_t) + pkg->n);
    uv_write(&req->req_, &stream->uvstream, &wrbuf, 1, &ti__write_cb);

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
    ti_write_t * ti_req;

    if (status)
        log_error(uv_strerror(status));

    ti_req = req->data;
    ti_req->cb_(ti_req, status ? EX_WRITE_UV : 0);
}
