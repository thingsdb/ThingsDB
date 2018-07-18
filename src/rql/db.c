/*
 * db.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <rql/api.h>
#include <rql/db.h>
#include <rql/elem.h>
#include <rql/elems.h>
#include <rql/inliners.h>
#include <rql/prop.h>
#include <rql/props.h>
#include <util/strx.h>
#include <util/fx.h>

static const size_t rql_db_min_name = 1;
static const size_t rql_db_max_name = 128;

rql_db_t * rql_db_create(rql_t * rql, guid_t * guid, const rql_raw_t * name)
{
    rql_db_t * db = malloc(sizeof(rql_db_t));
    if (!db)
        return NULL;

    db->ref = 1;
    db->rql = rql;
    db->root = NULL;
    db->name = rql_raw_dup(name);
    db->elems = imap_create();
    db->props = smap_create();
    db->access = vec_new(1);

    memcpy(&db->guid, guid, sizeof(guid_t));

    if (!db->name || !db->elems || !db->props || !db->access)
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
        vec_destroy(db->access, free);
        rql_elem_drop(db->root);
        rql_elems_gc(db->elems, NULL);
        assert (db->elems->n == 0);
        imap_destroy(db->elems, NULL);
        smap_destroy(db->props, free);
        free(db);
    }
}

int rql_db_buid(rql_db_t * db)
{
    rql_elem_t * elem = rql_elems_create(db->elems, rql_get_id(db->rql));
    if (!elem) return -1;

    if (rql_has_id(db->rql, elem->id))
    {
        rql_prop_t * prop = rql_db_props_get(db->props, "name");
        if (!prop || rql_elem_set(elem, prop, RQL_VAL_RAW, db->name))
            return -1;
    }
    db->root = rql_elem_grab(elem);

    return 0;
}

int rql_db_name_check(const rql_raw_t * name, ex_t * e)
{
    if (name->n < rql_db_min_name)
    {
        ex_set(e, -1, "database name should be at least %u characters",
                rql_db_min_name);
        return -1;
    }

    if (name->n >= rql_db_max_name)
    {
        ex_set(e, -1, "database name should be less than %u characters",
                rql_db_max_name);
        return -1;
    }

    if (!strx_is_graphn((const char *) name->data, name->n))
    {
        ex_set(e, -1,
                "database name should consist only of graphical characters");
        return -1;
    }

    if (name->data[0] == RQL_API_PREFIX[0])
    {
        ex_set(e, -1,
                "database name should not start with an `"RQL_API_PREFIX"`");
        return -1;
    }
    return 0;
}

int rql_db_store(rql_db_t * db, const char * fn)
{
    int rc;
    FILE * f = fopen(fn, "w");
    if (!f)
        return -1;

    rc = -(fwrite(&db->root->id, sizeof(uint64_t), 1, f) != 1);

    if (rc)
        log_error("saving failed: `%s`", fn);

    return -(fclose(f) || rc);
}

int rql_db_restore(rql_db_t * db, const char * fn)
{
    int rc = 0;
    ssize_t sz;
    unsigned char * data = fx_read(fn, &sz);
    if (!data || sz != sizeof(uint64_t)) goto failed;

    uint64_t id;
    memcpy(&id, data, sizeof(uint64_t));

    db->root = imap_get(db->elems, id);
    if (!db->root)
    {
        log_critical("cannot find root element: %"PRIu64, id);
        goto failed;
    }

    db->root = rql_elem_grab(db->root);
    goto done;

failed:
    rc = -1;
    log_critical("failed to restore from file: `%s`", fn);
done:
    free(data);
    return rc;
}
