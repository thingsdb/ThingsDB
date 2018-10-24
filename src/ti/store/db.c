/*
 * ti/store/db.c
 */

#include <ti/store/db.h>
#include <util/fx.h>
#include <stdlib.h>

static const char * ti__store_db_things_fn     = "things.dat";
static const char * ti__store_db_db_fn         = "db.dat";
static const char * ti__store_db_skeleton_fn   = "skeleton.qp";
static const char * ti__store_db_data_fn       = "data.qp";
static const char * ti__store_db_access_fn     = "access.qp";

ti_store_db_t * ti_store_db_create(const char * path, ti_db_t * db)
{
    char * db_path;
    ti_store_db_t * store_db = malloc(sizeof(ti_store_db_t));
    if (!store_db)
        goto fail0;

    db_path = store_db->db_path = fx_path_join(path, db->guid.guid);
    if (!db_path)
        goto fail0;

    store_db->access_fn = fx_path_join(db_path, ti__store_db_access_fn);
    store_db->things_fn = fx_path_join(db_path, ti__store_db_things_fn);
    store_db->db_fn = fx_path_join(db_path, ti__store_db_db_fn);
    store_db->skeleton_fn = fx_path_join(db_path, ti__store_db_skeleton_fn);
    store_db->data_fn = fx_path_join(db_path, ti__store_db_data_fn);

    if (    !store_db->access_fn ||
            !store_db->things_fn ||
            !store_db->db_fn ||
            !store_db->skeleton_fn ||
            !store_db->data_fn)
        goto fail1;

    return store_db;
fail1:
    ti_store_db_destroy(store_db);
    return NULL;
fail0:
    free(store_db);
    return NULL;
}


void ti_store_db_destroy(ti_store_db_t * store_db)
{
    if (!store_db)
        return;
    free(store_db->access_fn);
    free(store_db->things_fn);
    free(store_db->db_fn);
    free(store_db->skeleton_fn);
    free(store_db->data_fn);
    free(store_db->db_path);
    free(store_db);
}
