/*
 * db.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <rql/db.h>
#include <rql/elem.h>
#include <rql/elems.h>
#include <rql/api.h>
#include <rql/prop.h>
#include <rql/props.h>
#include <rql/inliners.h>

#include <util/strx.h>

static const size_t rql_db_min_name = 1;
static const size_t rql_db_max_name = 128;

rql_db_t * rql_db_create(rql_t * rql, guid_t * guid, const char * name)
{
    rql_db_t * db = (rql_db_t *) malloc(sizeof(rql_db_t));
    if (!db) return NULL;

    db->ref = 1;
    db->rql = rql;
    db->name = strdup(name);
    db->elems = imap_create();
    db->props = smap_create();
    memcpy(&db->guid, guid, sizeof(guid_t));

    if (!db->name || !db->elems || !db->props)
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
        imap_destroy(db->elems, (imap_destroy_cb) rql_elem_drop);
        smap_destroy(db->props, (smap_destroy_cb) rql_prop_drop);
        free(db);
    }
}

int rql_db_buid(rql_db_t * db)
{
    rql_elem_t elem = rql_elems_create(rql_get_id(db->rql));
    if (!elem) return -1;

    if (rql_has_id(db->rql, elem->id))
    {
        rql_prop_t * prop = rql_db_props_get(db->props, "name");
        if (!prop ||
            rql_elem_set(elem, prop, RQL_VAL_STR, db->name)) return -1;
    }

    db->root = rql_elem_grab(elem);
    return 0;
}

int rql_db_name_check(const char * name, ex_t * e)
{
    size_t n = strlen(name);
    if (n < rql_db_min_name)
    {
        ex_set(e, -1, "database name should be at least %u characters",
                rql_db_min_name);
        return -1;
    }

    if (n >= rql_db_max_name)
    {
        ex_set(e, -1, "database name should be less than %u characters",
                rql_db_max_name);
        return -1;
    }

    if (!strx_is_graph(name))
    {
        ex_set(e, -1,
                "database name should consist only of graphical characters");
        return -1;
    }

    if (name[0] == RQL_API_PREFIX[0])
    {
        ex_set(e, -1,
                "database name should not start with an ''");
        return -1;
    }
    return 0;
}
