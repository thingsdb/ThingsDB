/*
 * ti/watch.h
 */
#include <ti/watch.h>
#include <ti.h>
#include <ti/proto.h>
#include <assert.h>
#include <stdlib.h>
#include <util/qpx.h>


ti_watch_t * ti_watch_create(ti_stream_t * stream)
{
    ti_watch_t * watch = malloc(sizeof(ti_watch_t));
    if (!watch)
        return NULL;
    watch->stream = stream;
    return watch;
}

void ti_watch_drop(ti_watch_t * watch)
{
    if (!watch)
        return;
    if (watch->stream)
        watch->stream = NULL;
    else
        free(watch);
}

ti_pkg_t * ti_watch_pkg(
        uint64_t thing_id,
        uint64_t event_id,
        const unsigned char * jobs,
        size_t n)
{
    ti_pkg_t * pkg;
    qp_packer_t * packer = qpx_packer_create(n + 36, 2);
    if (!packer)
        return NULL;
    (void) qp_add_map(&packer);
    (void) qp_add_raw(packer, (const uchar *) TI_KIND_S_THING, 1);
    (void) qp_add_int(packer, thing_id);
    (void) qp_add_raw_from_str(packer, "event");
    (void) qp_add_int(packer, event_id);
    (void) qp_add_raw_from_str(packer, "jobs");
    (void) qp_add_array(&packer);
    (void) qp_add_qp(packer, jobs, n);
    (void) qp_close_array(packer);
    (void) qp_close_map(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_WATCH_UPD);

    qpx_log("generated task for subscribers:",
            pkg->data, pkg->n, LOGGER_DEBUG);

    return pkg;
}
