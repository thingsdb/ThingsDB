/*
 * qpx.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_QPX_H_
#define RQL_QPX_H_

#include <rql/pkg.h>
#include <qpack.h>

typedef qp_packer_t qpx_packer_t;

qp_res_t * qpx_map_get(qp_map_t * map, const char * key);
_Bool qpx_raw_equal(qp_obj_t * obj, const char * s);

qpx_packer_t * qpx_packer_create(size_t sz);
void qpx_packer_destroy(qpx_packer_t * xpkg);
rql_pkg_t * qpx_packer_pkg(qpx_packer_t * packer, uint8_t tp);
void qpx_unpacker_init(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        size_t len);

#endif /* RQL_QPX_H_ */
