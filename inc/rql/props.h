/*
 * props.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_PROPS_H_
#define RQL_PROPS_H_


#include <stdint.h>
#include <rql/prop.h>
#include <util/smap.h>
#include <util/imap.h>

rql_prop_t * rql_db_props_get(smap_t * props, const char * name);
int rql_props_gc(smap_t * props);
int rql_props_store(smap_t * props, const char * fn);
imap_t * rql_props_restore(smap_t * props, const char * fn);
#endif /* RQL_PROPS_H_ */
