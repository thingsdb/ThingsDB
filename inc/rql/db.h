/*
 * db.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_DB_H_
#define RQL_DB_H_

typedef struct rql_db_s  rql_db_t;

struct rql_db_s
{
    cleri_grammar_t * lang;
};

rql_t * rql_create(void);
void rql_destroy(rql_t * rql);

#endif /* RQL_DB_H_ */
