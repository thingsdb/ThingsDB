/*
 * pool.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/pool.h>
#include <rql/db.h>
#include <stdlib.h>


rql_pool_t * rql_pool_create(void)
{
    rql_pool_t * pool = (rql_pool_t *) malloc(sizeof(rql_pool_t));
    if (!pool) return NULL;

    pool->dbs = vec_create(0);
    pool->nodes = vec_create(0);

    if (!pool->dbs || !pool->nodes)
    {
        rql_pool_destroy(pool);
        return NULL;
    }

    return pool;
}

void rql_pool_destroy(rql_pool_t * pool)
{
    if (!pool) return;

    vec_destroy(pool->dbs, (vec_destroy_cb) rql_db_drop);
    vec_destroy(pool->nodes, NULL);

    free(pool);
}
