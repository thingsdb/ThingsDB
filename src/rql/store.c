/*
 * store.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/store.h>
#include <rql/users.h>
#include <rql/access.h>
#include <rql/props.h>
#include <rql/elems.h>
#include <rql/dbs.h>
#include <util/fx.h>
#include <util/imap.h>
#include <util/logger.h>

static const char * rql__store_tmp_path     = ".tmp/";
static const char * rql__store_prev_path    = ".prev/";
static const char * rql__store_path         = ".store/";
static const char * rql__store_access_fn    = "access.qp";
static const char * rql__store_dbs_fn       = "dbs.qp";
static const char * rql__store_users_fn     = "users.qp";
static const char * rql__store_rql_fn       = "rql.qp";
static const char * rql__store_props_fn     = "props.dat";
static const char * rql__store_elems_fn     = "elems.dat";
static const char * rql__store_db_fn        = "db.dat";
static const char * rql__store_skeleton_fn  = "skeleton.qp";
static const char * rql__store_data_fn      = "data.qp";


int rql_store(rql_t * rql)
{
    int rc = -1;
    char * rql_fn = NULL;
    char * users_fn = NULL;
    char * access_fn = NULL;
    char * props_fn = NULL;
    char * elems_fn = NULL;
    char * dbs_fn = NULL;
    char * db_path = NULL;
    char * db_fn = NULL;
    char * skeleton_fn = NULL;
    char * data_fn = NULL;

    char * tmp_path = fx_path_join(rql->cfg->rql_path, rql__store_tmp_path);
    char * prev_path = fx_path_join(rql->cfg->rql_path, rql__store_prev_path);
    char * store_path = fx_path_join(rql->cfg->rql_path, rql__store_path);
    if (!tmp_path || !prev_path || !store_path) goto stop;

    /* not need for checking on errors */
    fx_rmdir(prev_path);
    mkdir(tmp_path, 0700);

    rql_fn = fx_path_join(tmp_path, rql__store_rql_fn);
    users_fn = fx_path_join(tmp_path, rql__store_users_fn);
    access_fn = fx_path_join(tmp_path, rql__store_access_fn);
    dbs_fn = fx_path_join(tmp_path, rql__store_dbs_fn);

    if (!rql_fn || !users_fn || !access_fn || !dbs_fn ||
        rql__store(rql, rql_fn) ||
        rql_users_store(rql->users, users_fn) ||
        rql_access_store(rql->access, access_fn) ||
        rql_dbs_store(rql->dbs, dbs_fn)) goto stop;

    for (vec_each(rql->dbs, rql_db_t, db))
    {
        free(skeleton_fn);
        skeleton_fn = NULL;
        free(data_fn);
        data_fn = NULL;
        free(db_fn);
        db_fn = NULL;
        free(elems_fn);
        elems_fn = NULL;
        free(props_fn);
        props_fn = NULL;
        free(access_fn);
        access_fn = NULL;
        free(db_path);
        db_path = NULL;

        db_path = fx_path_join(tmp_path, db->guid.guid);
        if (!db_path || mkdir(db_path, 0700)) goto stop;

        access_fn = fx_path_join(db_path, rql__store_access_fn);
        props_fn = fx_path_join(db_path, rql__store_props_fn);
        elems_fn = fx_path_join(db_path, rql__store_elems_fn);
        db_fn = fx_path_join(db_path, rql__store_db_fn);
        skeleton_fn = fx_path_join(db_path, rql__store_skeleton_fn);
        data_fn = fx_path_join(db_path, rql__store_data_fn);

        if (!access_fn || !props_fn || !elems_fn || !db_fn ||
            rql_access_store(db->access, access_fn) ||
            rql_props_store(db->props, props_fn) ||
            rql_elems_store(db->elems, elems_fn) ||
            rql_db_store(db, db_fn) ||
            rql_elems_store_skeleton(db->elems, skeleton_fn) ||
            rql_elems_store_data(db->elems, data_fn)) goto stop;
    }

    rename(store_path, prev_path);
    if (rename(tmp_path, store_path)) goto stop;

    fx_rmdir(prev_path);

    rc = 0;

stop:
    if (rc && tmp_path)
    {
        log_error("storing rql has failed");
        fx_rmdir(tmp_path);
    }
    free(db_fn);
    free(skeleton_fn);
    free(data_fn);
    free(db_path);
    free(rql_fn);
    free(users_fn);
    free(access_fn);
    free(props_fn);
    free(elems_fn);
    free(dbs_fn);
    free(prev_path);
    free(store_path);
    free(tmp_path);
    return rc;
}

int rql_restore(rql_t * rql)
{
    int rc = -1;
    char * rql_fn = NULL;
    char * users_fn = NULL;
    char * access_fn = NULL;
    char * db_fn = NULL;
    char * skeleton_fn = NULL;
    char * data_fn = NULL;
    char * props_fn = NULL;
    char * elems_fn = NULL;
    char * dbs_fn = NULL;
    char * db_path = NULL;
    imap_t * propsmap = NULL;
    char * store_path = fx_path_join(rql->cfg->rql_path, rql__store_path);
    if (!store_path) goto stop;

    users_fn = fx_path_join(store_path, rql__store_users_fn);
    rql_fn = fx_path_join(store_path, rql__store_rql_fn);
    access_fn = fx_path_join(store_path, rql__store_access_fn);
    dbs_fn = fx_path_join(store_path, rql__store_dbs_fn);

    if (!users_fn || !rql_fn || !access_fn || !dbs_fn ||
        rql__restore(rql, rql_fn) ||
        rql_users_restore(&rql->users, users_fn) ||
        rql_access_restore(&rql->access, rql->users, access_fn) ||
        rql_dbs_restore(&rql->dbs, rql, dbs_fn)) goto stop;

    for (vec_each(rql->dbs, rql_db_t, db))
    {
        imap_destroy(propsmap, NULL);
        propsmap = NULL;
        free(db_fn);
        db_fn = NULL;
        free(skeleton_fn);
        skeleton_fn = NULL;
        free(data_fn);
        data_fn = NULL;
        free(elems_fn);
        elems_fn = NULL;
        free(props_fn);
        props_fn = NULL;
        free(access_fn);
        access_fn = NULL;
        free(db_path);
        db_path = NULL;

        db_path = fx_path_join(store_path, db->guid.guid);
        if (!db_path) goto stop;

        access_fn = fx_path_join(db_path, rql__store_access_fn);
        props_fn = fx_path_join(db_path, rql__store_props_fn);
        elems_fn = fx_path_join(db_path, rql__store_elems_fn);
        db_fn = fx_path_join(db_path, rql__store_db_fn);
        skeleton_fn = fx_path_join(db_path, rql__store_skeleton_fn);
        data_fn = fx_path_join(db_path, rql__store_data_fn);

        if (!access_fn || !props_fn || !elems_fn || !db_fn ||
            !(propsmap = rql_props_restore(db->props, props_fn)) ||
            rql_access_restore(&db->access, rql->users, access_fn) ||
            rql_elems_restore(db->elems, elems_fn) ||
            rql_db_restore(db, db_fn) ||
            rql_elems_restore_skeleton(db->elems, propsmap, skeleton_fn) ||
            rql_elems_restore_data(db->elems, propsmap, data_fn)) goto stop;
    }

    rc = 0;

stop:
    imap_destroy(propsmap, NULL);
    free(db_fn);
    free(skeleton_fn);
    free(data_fn);
    free(db_path);
    free(rql_fn);
    free(props_fn);
    free(elems_fn);
    free(users_fn);
    free(access_fn);
    free(dbs_fn);
    free(store_path);
    return rc;
}
