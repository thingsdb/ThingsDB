/*
 * db.c
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ti/api.h>
#include <ti/db.h>
#include <ti/things.h>
#include <ti/prop.h>
#include <ti/thing.h>
#include <ti/prop.h>
#include <ti/props.h>
#include <ti.h>
#include <util/strx.h>
#include <util/fx.h>

static const size_t ti_db_min_name = 1;
static const size_t ti_db_max_name = 128;

ti_db_t * ti_db_create(guid_t * guid, const ti_raw_t * name)
{
    ti_db_t * db = malloc(sizeof(ti_db_t));
    if (!db)
        return NULL;

    db->ref = 1;
    db->root = NULL;
    db->name = ti_raw_dup(name);
    db->things = imap_create();
    db->access = vec_new(1);
    db->limits = ti_limits_create();

    memcpy(&db->guid, guid, sizeof(guid_t));

    if (!db->name || !db->things || !db->access || !db->limits)
    {
        ti_db_drop(db);
        return NULL;
    }

    return db;
}

ti_db_t * ti_db_grab(ti_db_t * db)
{
    db->ref++;
    return db;
}

void ti_db_drop(ti_db_t * db)
{
    if (db && !--db->ref)
    {
        free(db->name);
        vec_destroy(db->access, free);
        ti_thing_drop(db->root);
        ti_things_gc(db->things, NULL);
        assert (db->things->n == 0);
        imap_destroy(db->things, NULL);
        ti_limits_destroy(db->limits);
        free(db);
    }
}

int ti_db_buid(ti_db_t * db)
{
    ti_thing_t * thing = ti_things_create_thing(db, ti_next_thing_id());
    if (!thing)
        return -1;

    db->root = thing;
    return 0;
}

int ti_db_name_check(const ti_raw_t * name, ex_t * e)
{
    if (name->n < ti_db_min_name)
    {
        ex_set(e, -1, "database name should be at least %u characters",
                ti_db_min_name);
        return -1;
    }

    if (name->n >= ti_db_max_name)
    {
        ex_set(e, -1, "database name should be less than %u characters",
                ti_db_max_name);
        return -1;
    }

    if (!strx_is_graphn((const char *) name->data, name->n))
    {
        ex_set(e, -1,
                "database name should consist only of graphical characters");
        return -1;
    }

    if (name->data[0] == TI_API_PREFIX[0])
    {
        ex_set(e, -1,
                "database name should not start with an `"TI_API_PREFIX"`");
        return -1;
    }
    return 0;
}

int ti_db_store(ti_db_t * db, const char * fn)
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

int ti_db_restore(ti_db_t * db, const char * fn)
{
    int rc = 0;
    ssize_t sz;
    unsigned char * data = fx_read(fn, &sz);
    if (!data || sz != sizeof(uint64_t)) goto failed;

    uint64_t id;
    memcpy(&id, data, sizeof(uint64_t));

    db->root = imap_get(db->things, id);
    if (!db->root)
    {
        log_critical("cannot find root thing: %"PRIu64, id);
        goto failed;
    }

    db->root = ti_thing_grab(db->root);
    goto done;

failed:
    rc = -1;
    log_critical("failed to restore from file: `%s`", fn);
done:
    free(data);
    return rc;
}
