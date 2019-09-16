/*
 * ti/warn.c
 */
#include <ti/warn.h>
#include <ti/proto.h>
#include <util/qpx.h>
#include <qpack.h>
#include <stdarg.h>

int ti_warn(ti_stream_t * stream, ti_warn_enum_t tp, const char * fmt, ...)
{
    int rc;
    va_list args;
    ti_pkg_t * pkg;
    qpx_packer_t * xpkg = qpx_packer_create(1024, 1);
    if (!xpkg)
        return -1;

    (void) qp_add_map(&xpkg);
    (void) qp_add_raw(xpkg, (const unsigned char *) "warn_code", 9);
    (void) qp_add_int(xpkg, tp);
    (void) qp_add_raw(xpkg, (const unsigned char *) "warn_msg", 8);

    va_start(args, fmt);
    rc = (qp_add_raw_from_fmt(xpkg, fmt, args) || qp_close_map(xpkg));
    va_end(args);

    if (rc)
        return -1;

    pkg = qpx_packer_pkg(xpkg, TI_PROTO_CLIENT_WARN);

    return ti_stream_write_pkg(stream, pkg);
}
