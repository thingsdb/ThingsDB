/*
 * store.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/store.h>
#include <util/fx.h>

const char * rql__store_tmp_path = ".tmp/";
const char * rql__store_prev_path = ".prev/";
const char * rql__store_path = ".store/";
const char * rql__store_users_fn = "users.qp";


int rql_store(rql_t * rql)
{
    char * tmp_users = NULL;
    char * tmp_path = strx_cat(rql->cfg->rql_path, rql__store_tmp_path);
    char * prev_path = strx_cat(rql->cfg->rql_path, rql__store_prev_path);
    char * store_path = strx_cat(rql->cfg->rql_path, rql__store_path);
    if (!tmp_path || !prev_path || !store_path) goto failed;

    /* not need for checking on errors */
    fx_rmdir(rql__store_prev_path);
    mkdir(tmp_path, 0700);

    /* store users */
    char * tmp_users = strx_cat(tmp_path, rql__store_users_fn);
    if (!tmp_users || rql_users_store(rql->users, tmp_users)) goto failed;

    rename(store_path, prev_path);
    if (rename(tmp_path, store_path)) goto failed;

    fx_rmdir(rql__store_prev_path);
    return 0;

failed:
    free(tmp_users);

    free(prev_path);
    free(store_path);
    fx_rmdir(tmp_path);
    free(tmp_path);

    return -1;
}

int rql_restore(rql_t * rql)
{
    char * store_path = strx_cat(rql->cfg->rql_path, rql__store_path);

}
