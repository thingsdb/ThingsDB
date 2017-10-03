/*
 * rql.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <inttypes.h>
#include <rql/rql.h>
#include <rql/db.h>
#include <lang/lang.h>

rql_t * rql_create(void)
{
    rql_t * rql = (rql_t *) malloc(sizeof(rql_t));
    if (!rql) return NULL;

    rql->lang = compile_grammar();
    rql->dbs = vec_create(0);

    if (!rql->lang || !rql->dbs)
    {
        rql_destroy(rql);
        return NULL;
    }

    return rql;
}

void rql_destroy(rql_t * rql)
{
    if (!rql) return;
    if (rql->lang)
    {
        cleri_grammar_free(rql->lang);
    }
    vec_destroy(rql->dbs, (vec_destroy_cb) rql_db_drop);
    free(rql);
}
