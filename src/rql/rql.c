/*
 * rql.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/rql.h>
#include <lang/lang.h>

rql_t * rql_create(void)
{
    rql_t * rql = (rql_t *) malloc(sizeof(rql_t));
    if (!rql) return NULL;

    rql->lang = compile_grammar();
   ;
    if (!rql->lang)
    {
        rql_destroy(rql);
        return NULL;
    }

    return rql;
}

void rql_destroy(rql_t * rql)
{
    if (rql->lang)
    {
        cleri_grammar_free(rql->lang);
    }
    free(rql);
}
