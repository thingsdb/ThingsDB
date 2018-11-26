/*
 * db.c
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ti/db.h>
#include <ti/things.h>
#include <ti/name.h>
#include <ti/auth.h>
#include <ti/thing.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti.h>
#include <util/strx.h>
#include <util/fx.h>

static const size_t ti_db_min_name = 1;
static const size_t ti_db_max_name = 128;

ti_db_t * ti_db_create(guid_t * guid, const char * name, size_t n)
{
    ti_db_t * db = malloc(sizeof(ti_db_t));
    if (!db)
        return NULL;

    db->ref = 1;
    db->root = NULL;
    db->name = ti_raw_create((uchar *) name, n);
    db->things = imap_create();
    db->access = vec_new(1);
    db->quota = ti_quota_create();
    db->lock = malloc(sizeof(uv_mutex_t));


    memcpy(&db->guid, guid, sizeof(guid_t));

    if (!db->name || !db->things || !db->access ||
        !db->quota || !db->lock || uv_mutex_init(db->lock))
    {
        ti_db_drop(db);
        return NULL;
    }

    return db;
}

void ti_db_drop(ti_db_t * db)
{
    if (db && !--db->ref)
    {
        free(db->name);
        vec_destroy(db->access, (vec_destroy_cb) ti_auth_destroy);

        ti_thing_drop(db->root);
        ti_things_gc(db->things, NULL, false);

        assert (db->things->n == 0);
        imap_destroy(db->things, NULL);
        ti_quota_destroy(db->quota);
        uv_mutex_destroy(db->lock);
        free(db->lock);
        free(db);
    }
}

_Bool ti_db_name_check(const char * name, size_t n, ex_t * e)
{
    if (n < ti_db_min_name || n >= ti_db_max_name)
    {
        ex_set(e, EX_BAD_DATA,
                "database name must be between %u and %u characters",
                ti_db_min_name,
                ti_db_max_name);
        return false;
    }

    if (!ti_name_is_valid_strn(name, n))
    {
        ex_set(e, EX_BAD_DATA,
                "database name should be a valid name, "
                "see "TI_DOCS"#names");
        return false;
    }

    return true;
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
    uchar * data = fx_read(fn, &sz);
    if (!data || sz != sizeof(uint64_t)) goto failed;

    uint64_t id;
    memcpy(&id, data, sizeof(uint64_t));

    db->root = imap_get(db->things, id);
    if (!db->root)
    {
        log_critical("cannot find root thing: %"PRIu64, id);
        goto failed;
    }

    db->root = ti_grab(db->root);
    goto done;

failed:
    rc = -1;
    log_critical("failed to restore from file: `%s`", fn);
done:
    free(data);
    return rc;
}
