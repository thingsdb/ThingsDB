/*
 * db.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/db.h>
#include <rql/elem.h>

rql_db_t * rql_db_create(void)
{
    rql_db_t * db = (rql_db_t *) malloc(sizeof(rql_db_t));
    if (!db) return NULL;

    db->ref = 1;
    db->name = NULL;
    db->elems = imap_create();
    db->props = smap_create();

    if (!db->elems || !db->props)
    {
        rql_db_drop(db);
        return NULL;
    }

    return db;
}

rql_db_t * rql_db_grab(rql_db_t * db)
{
    db->ref++;
    return db;
}

void rql_db_drop(rql_db_t * db)
{
    if (db && !--db->ref)
    {
        free(db->name);
        imap_destroy(db->elems, NULL);
        smap_destroy(db->props, NULL);
        free(db);
    }
}
