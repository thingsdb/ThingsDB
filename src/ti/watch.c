/*
 * ti/watch.h
 */
#include <ti/watch.h>
#include <ti.h>
#include <ti/proto.h>
#include <assert.h>
#include <stdlib.h>
#include <util/mpack.h>


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

ti_rpkg_t * ti_watch_rpkg(
        uint64_t thing_id,
        uint64_t event_id,
        const unsigned char * mpjobs,
        size_t size)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_rpkg_t * rpkg;
    ti_pkg_t * pkg;

    if (mp_sbuffer_alloc_init(&buffer, size + 64, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 3);

    mp_pack_strn(&pk, TI_KIND_S_THING, 1);
    msgpack_pack_uint64(&pk, thing_id);

    mp_pack_str(&pk, "event");
    msgpack_pack_uint64(&pk, event_id);

    mp_pack_str(&pk, "jobs");
    mp_pack_append(&pk, mpjobs, size);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_CLIENT_WATCH_UPD, buffer.size);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
    {
        free(pkg);
        return NULL;
    }

    return rpkg;
}
