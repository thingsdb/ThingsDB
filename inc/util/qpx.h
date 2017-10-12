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
static inline int qpx_raw_equal(qp_obj_t * obj, const char * s);


/*
 * Compare a raw object to a null terminated string.
 */
static inline int qpx_raw_equal(qp_obj_t * obj, const char * s)
{
    return !strncmp(s, obj->via.raw, obj->len) &&
            obj->via.raw[obj->len - 1] != '\0' &&
            s[obj->len] == '\0';
}

#endif /* RQL_QPX_H_ */
