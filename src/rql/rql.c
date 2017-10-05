/*
 * rql.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <inttypes.h>
#include <rql/rql.h>
#include <rql/db.h>
#include <util/logger.h>
#include <util/strx.h>
#include <string.h>
#include <stdlib.h>

rql_t * rql_create(void)
{
    rql_t * rql = (rql_t *) malloc(sizeof(rql_t));
    if (!rql) return NULL;

    rql->pool = rql_pool_create();
    rql->args = rql_args_create();

    if (!rql->pool || !rql->args)
    {
        rql_destroy(rql);
        return NULL;
    }

    return rql;
}

void rql_destroy(rql_t * rql)
{
    if (!rql) return;

    rql_pool_destroy(rql->pool);
    rql_args_destroy(rql->args);

    free(rql);
}

void rql_setup_logger(rql_t * rql)
{
    int n;
    char lname[255];
    size_t len = strlen(rql->args->log_level);

#ifndef DEBUG
    /* force colors while debugging... */
    if (rql->args->log_colorized)
#endif
    {
        Logger.flags |= LOGGER_FLAG_COLORED;
    }

    for (n = 0; n < LOGGER_NUM_LEVELS; n++)
    {
        strcpy(lname, LOGGER_LEVEL_NAMES[n]);
        strx_lower_case(lname);
        if (strlen(lname) == len && strcmp(rql->args->log_level, lname) == 0)
        {
            logger_init(stdout, n);
            return;
        }
    }
    /* We should not get here since args should always
     * contain a valid log level
     */
    logger_init(stdout, 0);
}
