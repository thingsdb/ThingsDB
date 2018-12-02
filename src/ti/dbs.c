/*
 * dbs.h
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ti/db.h>
#include <ti/dbs.h>
#include <ti/auth.h>
#include <ti/proto.h>
#include <ti/access.h>
#include <ti/things.h>
#include <ti.h>
#include <util/vec.h>

static ti_dbs_t * dbs = NULL;

int ti_dbs_create(void)
{
    dbs = malloc(sizeof(ti_dbs_t));
    if (!dbs)
        goto failed;

    dbs->vec = vec_new(1);
    if (!dbs->vec)
        goto failed;

    ti()->dbs = dbs;
    return 0;

failed:
    free(dbs);
    return -1;
}

void ti_dbs_destroy(void)
{
    if (!dbs)
        return;
    vec_destroy(dbs->vec, (vec_destroy_cb) ti_db_drop);
    free(dbs);
    ti()->dbs = dbs = NULL;
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
    if (!db || vec_push(&dbs->vec, db))
    {
        ex_set_alloc(e);
        goto fail0;
    }

    db->root = ti_things_create_thing(db->things, database_id);

    if (!db->root || ti_access_grant(&db->access, user, TI_AUTH_FULL))
    {
        ex_set_alloc(e);
        goto fail1;
    }

    return db;

fail1:
    (void *) vec_pop(dbs->vec);
fail0:
    ti_db_drop(db);
    return NULL;
}


ti_db_t * ti_dbs_get_by_raw(const ti_raw_t * raw)
{
    for (vec_each(dbs->vec, ti_db_t, db))
        if (ti_raw_equal(db->name, raw))
            return db;
    return NULL;
}

/* returns a weak reference */
ti_db_t * ti_dbs_get_by_strn(const char * str, size_t n)
{
    for (vec_each(dbs->vec, ti_db_t, db))
        if (ti_raw_equal_strn(db->name, str, n))
            return db;
    return NULL;
}

/* returns a weak reference */
ti_db_t * ti_dbs_get_by_id(const uint64_t id)
{
    for (vec_each(dbs->vec, ti_db_t, db))
        if (id == db->root->id)
            return db;
    return NULL;
}

/*
 * Returns a weak reference database based on a QPack object.
 * If the database is not found, then e will contain the reason why.
 * - if the database target is `root`, then the return value is NULL and
 *   e->nr is EX_SUCCESS
 */
ti_db_t * ti_dbs_get_by_qp_obj(qp_obj_t * obj, ex_t * e)
{
    ti_db_t * db = NULL;
    switch (obj->tp)
    {
    case QP_RAW:
        db = ti_dbs_get_by_strn((char *) obj->via.raw, obj->len);
        if (!db)
            ex_set(
                e,
                EX_INDEX_ERROR,
                "database `%.*s` not found",
                obj->len,
                (char *) obj->via.raw);
        break;
    case QP_INT64:
        if (!obj->via.int64)
            ex_set(e, EX_SUCCESS, "database target is root");
        else
        {
            uint64_t id = (uint64_t) obj->via.int64;
            db = ti_dbs_get_by_id(id);
            if (!db)
                ex_set(
                    e,
                    EX_INDEX_ERROR,
                    TI_DB_ID" not found",
                    id);
        }
        break;
    default:
        ex_set(e, EX_BAD_DATA, "expecting a `name` or `id` as target");
    }
    return db;
}

/*
 * Returns a weak reference database based on a ti_val_t.
 * If the database is not found, then e will contain the reason why.
 * - if the database target is `root`, then the return value is NULL and
 *   e->nr is EX_SUCCESS
 */
ti_db_t * ti_dbs_get_by_val(ti_val_t * val, ex_t * e)
{
    ti_db_t * db = NULL;
    switch (val->tp)
    {
    case TI_VAL_RAW:
        db = ti_dbs_get_by_strn(
                (const char *) val->via.raw->data, val->via.raw->n);
        if (!db)
            ex_set(
                e,
                EX_INDEX_ERROR,
                "database `%.*s` not found",
                val->via.raw->n,
                (char *) val->via.raw->data);
        break;
    case TI_VAL_INT:
        if (val->via.int_ == 0)
            ex_set(e, EX_SUCCESS, "database target is root");
        else
        {
            uint64_t id = (uint64_t) val->via.int_;
            db = ti_dbs_get_by_id(id);
            if (!db)
                ex_set(
                    e,
                    EX_INDEX_ERROR,
                    TI_DB_ID" not found",
                    id);
        }
        break;
    default:
        ex_set(e, EX_BAD_DATA, "expecting type `%s` or `%s` as target",
                ti_val_tp_str(TI_VAL_RAW), ti_val_tp_str(TI_VAL_INT));
    }
    return db;
}

