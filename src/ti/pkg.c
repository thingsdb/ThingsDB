/*
 * ti/pkg.c
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ti/pkg.h>
#include <ti/proto.h>
#include <util/mpack.h>
#include <util/logger.h>

ti_pkg_t * ti_pkg_new(
        uint16_t id,
        uint8_t tp,
        const unsigned char * data,
        uint32_t n)
{
    ti_pkg_t * pkg = malloc(sizeof(ti_pkg_t) + n);

    if (!pkg)
        return NULL;

    pkg->tp = tp;
    pkg->ntp = tp^0xff;
    pkg->n = n;
    pkg->id = id;

    memcpy(pkg->data, data, n);

    return pkg;
}

/*
 * `total_n` is the total pkg size including the header, not `pkg->n` which is
 *  only the data size
 */
void pkg_init(ti_pkg_t * pkg, uint16_t id, uint8_t tp, size_t total_n)
{
    pkg->id = id;
    pkg->n = total_n-sizeof(ti_pkg_t);
    pkg->tp = tp;
    pkg->ntp = tp^0xff;
}

ti_pkg_t * ti_pkg_dup(ti_pkg_t * pkg)
{
    size_t sz = sizeof(ti_pkg_t) + pkg->n;
    ti_pkg_t * dup = malloc(sz);
    if (!dup)
        return NULL;
    memcpy(dup, pkg, sz);
    return dup;
}

ti_pkg_t * ti_pkg_client_err(uint16_t id, ex_t * e)
{
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, 40 + e->n, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "error_code");
    msgpack_pack_int8(&pk, e->nr);

    mp_pack_str(&pk, "error_msg");
    mp_pack_strn(&pk, e->msg, e->n);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, id, TI_PROTO_CLIENT_RES_ERROR, buffer.size);

    return pkg;
}

ti_pkg_t * ti_pkg_node_err(uint16_t id, ex_t * e)
{
    return ti_pkg_new(
            id,
            TI_PROTO_NODE_ERR_RES,
            (const unsigned char *) e->msg,
            e->n);
}

void ti_pkg_log(ti_pkg_t * pkg)
{
    switch (pkg->tp)
    {
    case TI_PROTO_NODE_ERR_RES:
        log_error("error response on `package:%u`: %.*s",
                pkg->id, (int) pkg->n, (char *) pkg->data);
        break;
    default:
        log_info("package id: %u, type: %s", pkg->id, ti_proto_str(pkg->tp));
    }
}

void ti_pkg_set_tp(ti_pkg_t * pkg, uint8_t tp)
{
    pkg->tp = tp;
    pkg->ntp = tp ^ 0xff;
}
