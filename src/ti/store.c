/*
 * store.c
 */
#include <stdlib.h>
#include <ti/access.h>
#include <ti/dbs.h>
#include <ti/props.h>
#include <ti/store.h>
#include <ti/things.h>
#include <ti/users.h>
#include <ti.h>
#include <util/fx.h>
#include <util/imap.h>
#include <util/logger.h>

static const char * ti__store_tmp_path      = ".tmp/";
static const char * ti__store_prev_path     = ".prev/";
static const char * ti__store_path          = ".store/";
static const char * ti__store_access_fn     = "access.qp";
static const char * ti__store_dbs_fn        = "dbs.qp";
static const char * ti__store_users_fn      = "users.qp";
static const char * ti__store_ti_stat_fn    = "tistat.qp";
static const char * ti__store_props_fn      = "props.qp";
static const char * ti__store_things_fn     = "things.dat";
static const char * ti__store_db_fn         = "db.dat";
static const char * ti__store_skeleton_fn   = "skeleton.qp";
static const char * ti__store_data_fn       = "data.qp";


int ti_store_store(void)
{
    ti_t * thingsdb = ti_get();
    int rc = -1;
    char * ti_stat_fn = NULL;
    char * users_fn = NULL;
    char * access_fn = NULL;
    char * props_fn = NULL;
    char * things_fn = NULL;
    char * dbs_fn = NULL;
    char * db_path = NULL;
    char * db_fn = NULL;
    char * skeleton_fn = NULL;
    char * data_fn = NULL;

    char * tmp_path = fx_path_join(
            thingsdb->cfg->store_path,
            ti__store_tmp_path);
    char * prev_path = fx_path_join(
            thingsdb->cfg->store_path,
            ti__store_prev_path);
    char * store_path = fx_path_join(
            thingsdb->cfg->store_path,
            ti__store_path);
    if (!tmp_path || !prev_path || !store_path) goto stop;

    /* not need for checking on errors */
    fx_rmdir(prev_path);
    mkdir(tmp_path, 0700);

    ti_stat_fn = fx_path_join(tmp_path, ti__store_ti_stat_fn);
    users_fn = fx_path_join(tmp_path, ti__store_users_fn);
    access_fn = fx_path_join(tmp_path, ti__store_access_fn);
    dbs_fn = fx_path_join(tmp_path, ti__store_dbs_fn);

    if (!ti_stat_fn || !users_fn || !access_fn || !dbs_fn ||
        ti_store(ti_stat_fn) ||
        ti_props_store(props_fn) ||
        ti_users_store(users_fn) ||
        ti_access_store(thingsdb->access, access_fn) ||
        ti_dbs_store(dbs_fn))
        goto stop;

    for (vec_each(thingsdb->dbs, ti_db_t, db))
    {
        free(skeleton_fn);
        skeleton_fn = NULL;
        free(data_fn);
        data_fn = NULL;
        free(db_fn);
        db_fn = NULL;
        free(things_fn);
        things_fn = NULL;
        free(props_fn);
        props_fn = NULL;
        free(access_fn);
        access_fn = NULL;
        free(db_path);
        db_path = NULL;

        db_path = fx_path_join(tmp_path, db->guid.guid);
        if (!db_path || mkdir(db_path, 0700))
            goto stop;

        access_fn = fx_path_join(db_path, ti__store_access_fn);
        props_fn = fx_path_join(db_path, ti__store_props_fn);
        things_fn = fx_path_join(db_path, ti__store_things_fn);
        db_fn = fx_path_join(db_path, ti__store_db_fn);
        skeleton_fn = fx_path_join(db_path, ti__store_skeleton_fn);
        data_fn = fx_path_join(db_path, ti__store_data_fn);

        if (!access_fn || !props_fn || !things_fn || !db_fn ||
            ti_access_store(db->access, access_fn) ||
            ti_things_store(db->things, things_fn) ||
            ti_db_store(db, db_fn) ||
            ti_things_store_skeleton(db->things, skeleton_fn) ||
            ti_things_store_data(db->things, data_fn))
            goto stop;
    }

    rename(store_path, prev_path);
    if (rename(tmp_path, store_path))
        goto stop;

    fx_rmdir(prev_path);

    rc = 0;

stop:
    if (rc && tmp_path)
    {
        log_error("storing ThingsDB has failed");
        fx_rmdir(tmp_path);
    }
    free(db_fn);
    free(skeleton_fn);
    free(data_fn);
    free(db_path);
    free(ti_stat_fn);
    free(users_fn);
    free(access_fn);
    free(props_fn);
    free(things_fn);
    free(dbs_fn);
    free(prev_path);
    free(store_path);
    free(tmp_path);
    return rc;
}

int ti_store_restore(void)
{
    ti_t * thingsdb = ti_get();
    int rc = -1;
    char * ti_stat_fn = NULL;
    char * users_fn = NULL;
    char * access_fn = NULL;
    char * db_fn = NULL;
    char * skeleton_fn = NULL;
    char * data_fn = NULL;
    char * props_fn = NULL;
    char * things_fn = NULL;
    char * dbs_fn = NULL;
    char * db_path = NULL;
    imap_t * propsmap = NULL;
    char * store_path = fx_path_join(thingsdb->cfg->store_path, ti__store_path);
    if (!store_path)
        goto stop;

    users_fn = fx_path_join(store_path, ti__store_users_fn);
    ti_stat_fn = fx_path_join(store_path, ti__store_ti_stat_fn);
    access_fn = fx_path_join(store_path, ti__store_access_fn);
    dbs_fn = fx_path_join(store_path, ti__store_dbs_fn);

    if (    !users_fn || !ti_stat_fn || !access_fn || !dbs_fn ||
            ti_restore(ti_stat_fn) ||
            ti_users_restore(users_fn) ||
            ti_access_restore(&thingsdb->access, access_fn) ||
            ti_dbs_restore(dbs_fn))
        goto stop;

    for (vec_each(thingsdb->dbs, ti_db_t, db))
    {
        imap_destroy(propsmap, NULL);
        propsmap = NULL;
        free(db_fn);
        db_fn = NULL;
        free(skeleton_fn);
        skeleton_fn = NULL;
        free(data_fn);
        data_fn = NULL;
        free(things_fn);
        things_fn = NULL;
        free(props_fn);
        props_fn = NULL;
        free(access_fn);
        access_fn = NULL;
        free(db_path);
        db_path = NULL;

        db_path = fx_path_join(store_path, db->guid.guid);
        if (!db_path)
            goto stop;

        access_fn = fx_path_join(db_path, ti__store_access_fn);
        props_fn = fx_path_join(db_path, ti__store_props_fn);
        things_fn = fx_path_join(db_path, ti__store_things_fn);
        db_fn = fx_path_join(db_path, ti__store_db_fn);
        skeleton_fn = fx_path_join(db_path, ti__store_skeleton_fn);
        data_fn = fx_path_join(db_path, ti__store_data_fn);

        if (!access_fn || !props_fn || !things_fn || !db_fn ||
            !(propsmap = ti_props_restore(props_fn)) ||
            ti_access_restore(&db->access, access_fn) ||
            ti_things_restore(db->things, things_fn) ||
            ti_db_restore(db, db_fn) ||
            ti_things_restore_skeleton(db->things, propsmap, skeleton_fn) ||
            ti_things_restore_data(db->things, propsmap, data_fn))
            goto stop;
    }

    rc = 0;

stop:
    imap_destroy(propsmap, NULL);
    free(db_fn);
    free(skeleton_fn);
    free(data_fn);
    free(db_path);
    free(ti_stat_fn);
    free(props_fn);
    free(things_fn);
    free(users_fn);
    free(access_fn);
    free(dbs_fn);
    free(store_path);
    return rc;
}
