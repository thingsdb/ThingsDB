/*
 * ti/warn.c
 */
#include <ti/pkg.h>
#include <ti/stream.h>
#include <ti/warn.h>

/*
 * Stream can be a client or node stream.
 * As a protocol, use either TI_PROTO_CLIENT_WARN or TI_PROTO_NODE_FWD_WARN
 * For a node, use the client request PID to respond, otherwise TI_PROTO_EV_ID
 * Warn Code should be a code > 0.
 */
int ti_warn(
        ti_stream_t * stream,
        uint16_t pid,
        ti_proto_enum_t proto,
        ti_warn_enum_t warn_code,
        const void * data,
        size_t n)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;

    if (mp_sbuffer_alloc_init(&buffer, n+40, sizeof(ti_pkg_t)))
        return -1;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) msgpack_pack_map(&pk, 2);
    (void) mp_pack_str(&pk, "warn_code");
    (void) msgpack_pack_int8(&pk, warn_code);
    (void) mp_pack_str(&pk, "warn_msg");
    (void) mp_pack_strn(&pk, data, n);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, pid, proto, buffer.size);

    if (ti_stream_write_pkg(stream, pkg))
    {
        free(pkg);
        return -1;
    }
    return 0;
}
