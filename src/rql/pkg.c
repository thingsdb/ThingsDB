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
#include <rql/front.h>

rql_pkg_t * rql_pkg_new(uint8_t tp, const unsigned char * data, uint32_t n)
{
    rql_pkg_t * pkg = (rql_pkg_t *) malloc(sizeof(rql_pkg_t) + n);
    if (!pkg) return NULL;
    pkg->tp = tp;
    pkg->ntp = tp ^ 255;
    pkg->n = n;
    memcpy(pkg->data, data, n);
    return pkg;
}

rql_pkg_t * rql_pkg_e(ex_t * e, uint16_t id)
{
    assert (e && e->n && e->errnr >= RQL_FRONT_ERR && e->errnr < 255);
    qp_packer_t * packer = qp_packer_create(sizeof(rql_pkg_t) + 20 + e->n);
    if (!packer) return NULL;
    packer->len = sizeof(rql_pkg_t);

    qp_add_map(&packer);
    qp_add_raw(packer, "error_msg", 9);
    qp_add_raw(packer, e->errmsg, e->n);
    qp_close_map(packer);

    rql_pkg_t * pkg = (rql_pkg_t *) packer->buffer;
    packer->buffer = NULL;
    qp_packer_destroy(packer);

    pkg->id = id;
    pkg->n = e->n;
    pkg->tp = (uint8_t) e->errnr;
    pkg->ntp = pkg->tp ^ 255;
    return pkg;
}
