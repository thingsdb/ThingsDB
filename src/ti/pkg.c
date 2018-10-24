/*
 * pkg.c
 */
#include <assert.h>
#include <string.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti/pkg.h>
#include <ti/proto.h>
#include <util/qpx.h>

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
    pkg->ntp = tp ^ 255;
    pkg->n = n;
    pkg->id = id;

    memcpy(pkg->data, data, n);

    return pkg;
}

ti_pkg_t * ti_pkg_err(uint16_t id, ex_t * e)
{
    uint8_t tp;
    ti_pkg_t * pkg;
    qpx_packer_t * xpkg = qpx_packer_create(30 + e->n, 1);
    if (!xpkg)
        return NULL;
    switch (e->nr)
    {
    case EX_AUTH_ERROR:
        tp = TI_PROTO_CLIENT_ERR_AUTH;
        break;
    case EX_FORBIDDEN:
        tp = TI_PROTO_CLIENT_ERR_FORBIDDEN;
        break;
    case EX_INDEX_ERROR:
        tp = TI_PROTO_CLIENT_ERR_INDEX;
        break;
    case EX_BAD_DATA:
        tp = TI_PROTO_CLIENT_ERR_BAD_REQUEST;
        break;
    case EX_QUERY_ERROR:
        tp = TI_PROTO_CLIENT_ERR_QUERY;
        break;
    case EX_NODE_ERROR:
        tp = TI_PROTO_CLIENT_ERR_NODE;
        break;
    case EX_INTERNAL:
    case EX_REQUEST_TIMEOUT:
    case EX_REQUEST_CANCEL:
    case EX_WRITE_UV:
    case EX_ALLOC:
    default:
        tp = TI_PROTO_CLIENT_ERR_INTERNAL;
    }

    (void) qp_add_map(&xpkg);
    (void) qp_add_raw(xpkg, (const unsigned char *) "error_code", 10);
    (void) qp_add_int64(xpkg, e->nr);
    (void) qp_add_raw(xpkg, (const unsigned char *) "error_msg", 9);
    (void) qp_add_raw(xpkg, (const unsigned char *) e->msg, e->n);
    (void) qp_close_map(xpkg);

    pkg = qpx_packer_pkg(xpkg, tp);
    pkg->id = id;

    return pkg;
}
