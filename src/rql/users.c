/*
 * users.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/users.h>
#include <rql/proto.h>
#include <util/cryptx.h>

const int rql_users_fn_schema = 0;

rql_user_t * rql_users_auth(
        vec_t * users,
        qp_obj_t * name,
        qp_obj_t * pass,
        ex_t * e)
{
    char passbuf[rql_max_pass];
    char pw[CRYPTX_SZ];

    if (name->len < rql_min_name || name->len >= rql_max_name)
    {
        ex_set(e, RQL_PROTO_AUTH_ERR, "invalid user");
        return NULL;
    }

    if (pass->len < rql_min_pass || pass->len >= rql_max_pass)
    {
        ex_set(e, RQL_PROTO_AUTH_ERR, "invalid password");
        return NULL;
    }

    for (vec_each(users, rql_user_t, user))
    {
        if (strncmp(user->name, name->via.raw, name->len) == 0)
        {
            memcpy(passbuf, pass->via.raw, pass->len);
            passbuf[pass->len] = '\0';

            cryptx(passbuf, user->pass, pw);
            if (strcmp(pw, user->pass))
            {
                ex_set(e, RQL_PROTO_AUTH_ERR, "incorrect password");
                return NULL;
            }
            return user;
        }
    }

    ex_set(e, RQL_PROTO_AUTH_ERR, "user not found");
    return NULL;
}

int rql_users_store(const vec_t * users, const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer) return -1;

    if (qp_add_map(&packer)) goto stop;

    /* schema */
    if (qp_add_raw(packer, "schema", 6) ||
        qp_add_int64(packer, rql_users_fn_schema)) goto stop;

    if (qp_add_raw(packer, "users", 5) ||
        qp_add_array(&packer)) goto stop;

    for (vec_each(users, rql_user_t, user))
    {
        if (qp_add_array(&packer) ||
            qp_add_raw(packer, user->name, strlen(user->name)) ||
            qp_add_raw(packer, user->pass, strlen(user->pass)) ||
            qp_close_array(packer)) goto stop;
    }

    if (qp_close_array(packer) || qp_close_map(packer)) goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);
    if (rc) log_error("failed to write file: '%s'", fn);

stop:
    qp_packer_destroy(packer);
    return rc;
}

int rql_users_restore(vec_t ** users, const char * fn)
{
    int rc = -1;
    ssize_t n;
    unsigned char * data = fx_read(fn, &n);
    if (!data) return -1;

    qp_unpacker_t unpacker;
    qp_unpacker_init(&unpacker, data, (size_t) n);
    qp_res_t * res = qp_unpacker_res(&unpacker, &rc);
    free(data);

    if (rc)
    {
        log_critical(qp_strerror(rc));
        return -1;
    }

    qp_res_t * schema, * qusers;

    if (res->tp != QP_RES_MAP ||
        !(schema = qpx_map_get(res->via.map, "schema")) ||
        !(qusers = qpx_map_get(res->via.map, "users")) ||
        schema->tp != QP_RES_INT64 ||
        schema->via.int64 != rql_users_fn_schema ||
        qusers->tp != QP_RES_ARRAY) goto stop;

    for (uint32_t i = 0; i < qusers->via.array->n; i++)
    {
        qp_res_t * quser = qusers->via.array->values + i;
        qp_res_t * name, * pass;
        if (quser->tp != QP_RES_ARRAY ||
                quser->via.array->n != 2 ||
            !(name = quser->via.array->values) ||
            !(pass = quser->via.array->values + 1) ||
            name->tp != QP_RES_STR ||
            pass->tp != QP_RES_STR) goto stop;

        rql_user_t * user = rql_user_create(name->via.str, pass->via.str);
        if (!user || vec_push(users, user)) goto stop;
    }

    rc = 0;

stop:
    qp_res_destroy(res);
    return rc;
}
