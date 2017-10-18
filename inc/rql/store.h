/*
 * store.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_STORE_H_
#define RQL_STORE_H_

#include <rql/rql.h>

int rql_store(rql_t * rql);
int rql_restore(rql_t * rql);

#endif /* RQL_STORE_H_ */
