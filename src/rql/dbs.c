/*
 * dbs.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <rql/dbs.h>

rql_db_t * rql_dbs_get_by_name(const vec_t * dbs, const char * name)
{
    for (vec_each(dbs, rql_db_t, db))
    {
        if (strcmp(db->name, name) == 0) return db;
    }
    return NULL;
}
