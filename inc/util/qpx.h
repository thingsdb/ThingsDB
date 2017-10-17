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
rql_pkg_t * qpx_packer_pkg(qpx_packer_t * packer, uint8_t tp);

#endif /* RQL_QPX_H_ */
