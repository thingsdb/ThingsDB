/*
 * dbs.h
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ti/db.h>
#include <ti/dbs.h>
#include <ti/proto.h>
#include <ti.h>
#include <util/vec.h>

static vec_t ** dbs = NULL;

int ti_dbs_create(void)
{
    ti()->dbs = vec_new(0);
    dbs = &ti()->dbs;
    return -(*dbs == NULL);
}

void ti_dbs_destroy(void)
{
    if (!dbs)
        return;
    vec_destroy(*dbs, (vec_destroy_cb) ti_db_drop);
    *dbs = NULL;
}

ti_db_t * ti_dbs_create_db(
        const char * name,
        size_t n,
        ti_user_t * user,
        ex_t * e)
{
    guid_t guid;
    uint64_t database_id;
    ti_db_t * db = NULL;
    if (ti_dbs_get_by_strn(name, n))
    {
        ex_set(e, EX_INDEX_ERROR,
                "database `%.*s` already exists", (int) n, name);
        goto fail0;
    }

    database_id = ti_next_thing_id();
    guid_init(&guid, database_id);

    db = ti_db_create(&guid, name, n);
    if (!db || vec_push(dbs, db))
    {
        ex_set_alloc(e);
        goto fail0;
    }

    if (ti_access_grant(&db->access, user, TI_AUTH_MASK_FULL) ||
        ti_db_buid(db))
    {
        ex_set_alloc(e);
        goto fail1;
    }

    return db;

fail1:
    (void *) vec_pop(*dbs);
fail0:
    ti_db_drop(db);
    return NULL;
}


ti_db_t * ti_dbs_get_by_raw(const ti_raw_t * raw)
{
    for (vec_each(*dbs, ti_db_t, db))
    {
        if (ti_raw_equal(db->name, raw))
            return db;
    }
    return NULL;
}

ti_db_t * ti_dbs_get_by_strn(const char * str, size_t n)
{
    for (vec_each(*dbs, ti_db_t, db))
    {
        if (ti_raw_equal_strn(db->name, str, n))
            return db;
    }
    return NULL;
}

ti_db_t * ti_dbs_get_by_id(const uint64_t id)
{
    for (vec_each(*dbs, ti_db_t, db))
    {
        if (id == db->root->id)
            return db;
    }
    return NULL;
}

/*
 * Returns a database based on a QPack object. If the database is not found,
 * then e will contain the reason why.
 */
ti_db_t * ti_dbs_get_by_qp_obj(qp_obj_t * obj, ex_t * e)
{
    ti_db_t * db = NULL;
    switch (obj->tp)
    {
    case QP_RAW:
        {
            db = ti_dbs_get_by_strn((char *) obj->via.raw, obj->len);
            if (!db)
                ex_set(
                    e,
                    EX_INDEX_ERROR,
                    "database with name `%.*s` is not found",
                    obj->len,
                    (char *) obj->via.raw);
        }
        break;
    case QP_INT64:
        if (!obj->via.int64)
        {
            ex_set(e, EX_SUCCESS, "database target is root");
        }
        else
        {
            uint64_t id = (uint64_t) obj->via.int64;
            db = ti_dbs_get_by_id(id);
            if (!db)
            {
                ex_set(
                    e,
                    EX_INDEX_ERROR,
                    "database with id `%"PRIu64"` is not found",
                    id);
            }
        }
        break;
    default:
        ex_set(e, EX_BAD_DATA, "expecting a `name` or `id` as target");
    }
    return db;
}
