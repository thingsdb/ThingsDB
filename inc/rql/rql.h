/*
 * rql.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_H_
#define RQL_H_

typedef struct rql_s  rql_t;

#include <cleri/cleri.h>

struct rql_s
{
    cleri_grammar_t * lang;
};

rql_t * rql_create(void);
void rql_destroy(rql_t * rql);

#endif /* RQL_H_ */
