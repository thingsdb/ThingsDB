/*
 * pkg.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <string.h>
#include <qpack.h>
#include <stdlib.h>
#include <rql/pkg.h>
#include <rql/proto.h>
#include <util/qpx.h>

rql_pkg_t * rql_pkg_new(
        uint16_t id,
        uint8_t tp,
        const unsigned char * data,
        uint32_t n)
{
    rql_pkg_t * pkg = (rql_pkg_t *) malloc(sizeof(rql_pkg_t) + n);
    if (!pkg) return NULL;
    pkg->tp = tp;
    pkg->ntp = tp ^ 255;
    pkg->n = n;
    pkg->id = id;
    memcpy(pkg->data, data, n);
    return pkg;
}

rql_pkg_t * rql_pkg_err(uint16_t id, uint8_t tp, const char * errmsg)
{
    size_t n = strlen(errmsg);
    qpx_packer_t * xpkg = qpx_packer_create(20 + n);
    if (!xpkg) return NULL;

    qp_add_map(&xpkg);
    qp_add_raw(xpkg, (const unsigned char *) "error_msg", 9);
    qp_add_raw(xpkg, (const unsigned char *) errmsg, n);
    qp_close_map(xpkg);

    rql_pkg_t * pkg = qpx_packer_pkg(xpkg, tp);
    pkg->id = id;
    return pkg;
}
