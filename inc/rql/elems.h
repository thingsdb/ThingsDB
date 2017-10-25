/*
 * elems.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ELEMS_H_
#define RQL_ELEMS_H_


#include <inttypes.h>
#include <rql/elem.h>
#include <util/imap.h>

rql_elem_t * rql_elems_create(imap_t * elems, uint64_t id);

struct rql_elems_s
{
    rql_prop_t * prop;
    rql_val_t val;
};

#endif /* RQL_ELEMS_H_ */
