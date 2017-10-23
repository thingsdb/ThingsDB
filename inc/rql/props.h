/*
 * props.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_PROPS_H_
#define RQL_PROPS_H_


#include <inttypes.h>
#include <rql/prop.h>
#include <util/smap.h>

rql_prop_t * rql_db_props_get(smap_t * props, rql_raw_t * raw);

#endif /* RQL_PROPS_H_ */
