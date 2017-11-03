/*
 * elems.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ELEMS_H_
#define RQL_ELEMS_H_


#include <stdint.h>
#include <rql/elem.h>
#include <util/imap.h>

rql_elem_t * rql_elems_create(imap_t * elems, uint64_t id);
int rql_elems_gc(imap_t * elems, rql_elem_t * root);
int rql_elems_store(imap_t * elems, const char * fn);
int rql_elems_store_skeleton(imap_t * elems, const char * fn);
int rql_elems_store_data(imap_t * elems, const char * fn);
int rql_elems_restore(imap_t * elems, const char * fn);
int rql_elems_restore_skeleton(imap_t * elems, imap_t * props, const char * fn);
int rql_elems_restore_data(imap_t * elems, imap_t * props, const char * fn);

struct rql_elems_s
{
    rql_prop_t * prop;
    rql_val_t val;
};

#endif /* RQL_ELEMS_H_ */
