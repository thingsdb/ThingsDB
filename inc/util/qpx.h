/*
 * qpx.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_QPX_H_
#define RQL_QPX_H_

#include <qpack.h>

qp_res_t * qpx_map_get(qp_map_t * map, const char * key);

#endif /* RQL_QPX_H_ */
