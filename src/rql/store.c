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
#include <util/fx.h>
#include <util/strx.h>
#include <util/logger.h>

static const char * rql__store_tmp_path     = ".tmp/";
static const char * rql__store_prev_path    = ".prev/";
static const char * rql__store_path         = ".store/";
static const char * rql__store_access_fn    = "access.qp";
static const char * rql__store_users_fn     = "users.qp";
static const char * rql__store_rql_fn       = "rql.qp";

int rql_store(rql_t * rql)
{
    int rc = -1;
    char * rql_fn = NULL;
    char * users_fn = NULL;
    char * access_fn = NULL;
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

    if (!rql_fn || !users_fn || !access_fn ||
        rql__store(rql, rql_fn) ||
        rql_users_store(rql->users, users_fn) ||
        rql_access_store(rql->access, access_fn)) goto stop;

    rename(store_path, prev_path);
    if (rename(tmp_path, store_path)) goto stop;

    fx_rmdir(prev_path);

    rc = 0;

stop:
    if (rc && tmp_path)
    {
        log_erorr("storing rql has failed");
        fx_rmdir(tmp_path);
    }
    free(rql_fn);
    free(users_fn);
    free(access_fn);
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
    char * store_path = strx_cat(rql->cfg->rql_path, rql__store_path);
    if (!store_path) goto stop;

    users_fn = strx_cat(store_path, rql__store_users_fn);
    rql_fn = strx_cat(store_path, rql__store_rql_fn);
    access_fn = strx_cat(store_path, rql__store_access_fn);

    if (!users_fn || !rql_fn || !access_fn ||
        rql__restore(rql->events, rql_fn) ||
        rql_users_restore(&rql->users, users_fn) ||
        rql_access_restore(&rql->access, rql->users, access_fn)) goto stop;

    rc = 0;

stop:
    free(rql_fn);
    free(users_fn);
    free(access_fn);
    free(store_path);
    return rc;
}
