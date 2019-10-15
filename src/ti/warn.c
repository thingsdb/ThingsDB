/*
 * ti/warn.c
 */
#include <ti/warn.h>
#include <ti/proto.h>
#include <util/mpack.h>
#include <stdarg.h>

int ti_warn(ti_stream_t * stream, ti_warn_enum_t tp, const char * fmt, ...)
{
    int rc;
    va_list args;
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (!ti_stream_is_client(stream))
    {
        /* just log the warning if the steam is not a client */
        va_list args;
        va_start(args, fmt);
        log_warning(fmt, args);
        va_end(args);
        return 0;
    }

    if (mp_sbuffer_alloc_init(&buffer, 1024, sizeof(ti_pkg_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "warn_code");
    msgpack_pack_int8(&pk, tp);

    mp_pack_str(&pk, "warn_msg");

    va_start(args, fmt);
    rc = mp_pack_fmt(&pk, fmt, args);
    va_end(args);

    if (rc)
        return -1;

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_WARN, buffer.size);

    return ti_stream_write_pkg(stream, pkg);
}
