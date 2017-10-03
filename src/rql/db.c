/*
 * db.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/db.h>
#include <rql/elem.h>
#include <rql/kind.h>

rql_db_t * rql_db_create(void)
{
    rql_db_t * db = (rql_db_t *) malloc(sizeof(rql_db_t));
    if (!db) return NULL;

    db->ref = 1;
    db->name = NULL;
    db->elems = imap_create();
    db->kinds = smap_create();

    if (!db->elems || !db->kinds)
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
    if (!--db->ref)
    {
        free(db->name);
        imap_destroy(db->elems, (imap_destroy_cb) rql_elem_drop);
        smap_destroy(db->kinds, (smap_destroy_cb) rql_kind_drop);
        free(db);
    }
}
