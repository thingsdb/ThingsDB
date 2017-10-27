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
#include <util/strx.h>
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

    char * tmp_path = strx_cat(rql->cfg->rql_path, rql__store_tmp_path);
    char * prev_path = strx_cat(rql->cfg->rql_path, rql__store_prev_path);
    char * store_path = strx_cat(rql->cfg->rql_path, rql__store_path);
    if (!tmp_path || !prev_path || !store_path) goto stop;

    /* not need for checking on errors */
    fx_rmdir(prev_path);
    mkdir(tmp_path, 0700);

    rql_fn = strx_cat(tmp_path, rql__store_rql_fn);
    users_fn = strx_cat(tmp_path, rql__store_users_fn);
    access_fn = strx_cat(tmp_path, rql__store_access_fn);
    dbs_fn = strx_cat(tmp_path, rql__store_dbs_fn);

    if (!rql_fn || !users_fn || !access_fn || !dbs_fn ||
        rql__store(rql, rql_fn) ||
        rql_users_store(rql->users, users_fn) ||
        rql_access_store(rql->access, access_fn) ||
        rql_dbs_store(rql->dbs, dbs_fn)) goto stop;

    for (vec_each(rql->dbs, rql_db_t, db))
    {
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

        db_path = strx_cat(tmp_path, db->guid.guid);
        if (!db_path || mkdir(db_path, 0700)) goto stop;

        access_fn = strx_cat(db_path, rql__store_access_fn);
        props_fn = strx_cat(db_path, rql__store_props_fn);
        elems_fn = strx_cat(db_path, rql__store_elems_fn);
        db_fn = strx_cat(db_path, rql__store_db_fn);

        if (!access_fn || !props_fn || !elems_fn || !db_fn ||
            rql_access_store(db->access, access_fn) ||
            rql_props_store(db->props, props_fn) ||
            rql_elems_store(db->elems, elems_fn) ||
            rql_db_store(db, db_fn)) goto stop;
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
    char * props_fn = NULL;
    char * elems_fn = NULL;
    char * dbs_fn = NULL;
    char * db_path = NULL;
    imap_t * propsmap = NULL;
    char * store_path = strx_cat(rql->cfg->rql_path, rql__store_path);
    if (!store_path) goto stop;

    users_fn = strx_cat(store_path, rql__store_users_fn);
    rql_fn = strx_cat(store_path, rql__store_rql_fn);
    access_fn = strx_cat(store_path, rql__store_access_fn);
    dbs_fn = strx_cat(store_path, rql__store_dbs_fn);

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
        free(elems_fn);
        elems_fn = NULL;
        free(props_fn);
        props_fn = NULL;
        free(access_fn);
        access_fn = NULL;
        free(db_path);
        db_path = NULL;

        db_path = strx_cat(store_path, db->guid.guid);
        if (!db_path) goto stop;

        access_fn = strx_cat(db_path, rql__store_access_fn);
        props_fn = strx_cat(db_path, rql__store_props_fn);
        elems_fn = strx_cat(db_path, rql__store_elems_fn);
        db_fn = strx_cat(db_path, db_fn);

        if (!access_fn || !props_fn || !elems_fn || db_fn ||
            !(propsmap = rql_props_restore(db->props, props_fn)) ||
            rql_access_restore(&db->access, rql->users, access_fn) ||
            rql_elems_restore(db->elems, elems_fn) ||
            rql_db_restore(db, db_fn)) goto stop;
    }

    rc = 0;

stop:
    imap_destroy(propsmap, NULL);
    free(db_fn);
    free(db_path);
    free(rql_fn);
    free(props_fn);
    free(users_fn);
    free(access_fn);
    free(dbs_fn);
    free(store_path);
    return rc;
}
