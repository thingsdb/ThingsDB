/*
 * store.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/dbs.h>
#include <ti/names.h>
#include <ti/store.h>
#include <ti/things.h>
#include <ti/users.h>
#include <ti/store/status.h>
#include <ti/store/access.h>
#include <ti/store/db.h>
#include <ti.h>
#include <util/fx.h>
#include <util/imap.h>
#include <util/logger.h>

/* path names */
static const char * ti__store_path          = ".store/";
static const char * ti__store_prev_path     = ".prev_/";
static const char * ti__store_tmp_path      = ".tmp__/";
/* file names */
static const char * ti__store_access_fn     = "access.qp";
static const char * ti__store_dbs_fn        = "dbs.qp";
static const char * ti__store_id_stat_fn    = "idstat.qp";
static const char * ti__store_names_fn      = "names.qp";
static const char * ti__store_users_fn      = "users.qp";

static void ti__store_set_filename(_Bool use_tmp);

static ti_store_t * store;

int ti_store_create(void)
{
    char * storage_path = ti_get()->cfg->storage_path;
    assert (storage_path);
    store = malloc(sizeof(ti_store_t));
    if (!store)
        goto fail0;

    /* path names */
    store->tmp_path = fx_path_join(storage_path, ti__store_tmp_path);
    if (!store->tmp_path)
        goto fail0;

    store->fn_offset = strlen(store->tmp_path) - strlen(ti__store_tmp_path);

    store->prev_path = fx_path_join(storage_path, ti__store_prev_path);
    store->store_path = fx_path_join(storage_path, ti__store_path);

    /* file names */
    store->access_fn = fx_path_join(store->tmp_path, ti__store_access_fn);
    store->dbs_fn = fx_path_join(store->tmp_path, ti__store_dbs_fn);
    store->id_stat_fn = fx_path_join(store->tmp_path, ti__store_id_stat_fn);
    store->names_fn = fx_path_join(store->tmp_path, ti__store_names_fn);
    store->users_fn = fx_path_join(store->tmp_path, ti__store_users_fn);

    if (    !store->prev_path ||
            !store->store_path ||
            !store->access_fn ||
            !store->dbs_fn ||
            !store->id_stat_fn ||
            !store->names_fn ||
            !store->users_fn)
        goto fail1;

    ti_get()->store = store;
    return 0;
fail1:
    ti_store_destroy();
fail0:
    free(store);
    store = NULL;
    return -1;
}

void ti_store_destroy(void)
{
    if (!store)
        return;
    free(store->prev_path);
    free(store->store_path);
    free(store->tmp_path);

    free(store->access_fn);
    free(store->dbs_fn);
    free(store->id_stat_fn);
    free(store->names_fn);
    free(store->users_fn);
    free(store);
    store = NULL;
}

int ti_store_store(void)
{
    assert (store);
    ti_t * ti = ti_get();

    /* not need for checking on errors */
    fx_rmdir(store->prev_path);
    mkdir(store->tmp_path, 0700);

    ti__store_set_filename(true);

    if (    ti_store_status_store(store->id_stat_fn) ||
            ti_names_store(store->names_fn) ||
            ti_users_store(store->users_fn) ||
            ti_store_access_store(ti->access, store->access_fn) ||
            ti_dbs_store(store->dbs_fn))
        goto failed;

    for (vec_each(ti->dbs, ti_db_t, db))
    {
        int rc;
        ti_store_db_t * store_db = ti_store_db_create(store->tmp_path, db);
        if (!store_db)
            goto failed;

        rc = (  mkdir(store_db->db_path, 0700) ||
                ti_store_access_store(db->access, store_db->access_fn) ||
                ti_things_store(db->things, store_db->things_fn) ||
                ti_db_store(db, store_db->db_fn) ||
                ti_things_store_skeleton(db->things, store_db->skeleton_fn) ||
                ti_things_store_data(db->things, store_db->data_fn));

        ti_store_db_destroy(store_db);
        if (rc)
            goto failed;
    }

    (void) rename(store->store_path, store->prev_path);
    if (rename(store->tmp_path, store->store_path))
        goto failed;

    (void) fx_rmdir(store->prev_path);

    return 0;

failed:
    log_error("storing ThingsDB has failed");
    (void) fx_rmdir(store->tmp_path);
    return -1;
}

int ti_store_restore(void)
{
    assert (store);
    ti_t * ti = ti_get();

    ti__store_set_filename(false);

    imap_t * namesmap = ti_names_restore(store->names_fn);
    int rc = (
            -(!namesmap) ||
            ti_store_status_restore(store->id_stat_fn) ||
            ti_users_restore(store->users_fn) ||
            ti_store_access_restore(&ti->access, store->access_fn) ||
            ti_dbs_restore(store->dbs_fn));

    if (rc)
        goto stop;

    for (vec_each(ti->dbs, ti_db_t, db))
    {
        ti_store_db_t * store_db = ti_store_db_create(store->store_path, db);
        rc = (  -(!store_db) ||
                ti_store_access_restore(&db->access, store_db->access_fn) ||
                ti_things_restore(db->things, store_db->things_fn) ||
                ti_db_restore(db, store_db->db_fn) ||
                ti_things_restore_skeleton(db->things, namesmap, store_db->skeleton_fn) ||
                ti_things_restore_data(db->things, namesmap, store_db->data_fn));
        if (rc)
            goto stop;
    }

stop:
    if (namesmap)
        imap_destroy(namesmap, NULL);
    return rc;
}

static void ti__store_set_filename(_Bool use_tmp)
{
    const char * path = use_tmp ? ti__store_tmp_path : ti__store_path;
    size_t n = strlen(path);
    memcpy(store->access_fn + store->fn_offset, path, n);
    memcpy(store->dbs_fn + store->fn_offset, path, n);
    memcpy(store->id_stat_fn + store->fn_offset, path, n);
    memcpy(store->names_fn + store->fn_offset, path, n);
    memcpy(store->users_fn + store->fn_offset, path, n);
}


