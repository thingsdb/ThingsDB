/*
 * access.c
 */
#include <qpack.h>
#include <stdlib.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <users.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/fx.h>

static const int ti__access_fn_schema = 0;

int ti_access_grant(vec_t ** access, ti_user_t * user, uint64_t mask)
{
    ti_auth_t * auth;
    for (vec_each(*access, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            auth->mask |= mask;
            return 0;
        }
    }
    auth = ti_auth_new(user, mask);
    if (!auth || vec_push(access, auth))
    {
        free(auth);
        return -1;
    }

    return 0;
}

void ti_access_revoke(vec_t * access, ti_user_t * user, uint64_t mask)
{
    uint32_t i = 0;
    for (vec_each(access, ti_auth_t, auth), i++)
    {
        if (auth->user == user)
        {
            auth->mask &= ~mask;
            if (!auth->mask)
            {
                vec_remove(access, i);
            }
        }
    }
}

_Bool ti_access_check(const vec_t * access, ti_user_t * user, uint64_t mask)
{
    for (vec_each(access, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            return (auth->mask & mask) == mask;
        }
    }
    return 0;
}

int ti_access_store(const vec_t * access, const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer) return -1;

    if (qp_add_map(&packer)) goto stop;

    /* schema */
    if (qp_add_raw_from_str(packer, "schema") ||
        qp_add_int64(packer, ti__access_fn_schema)) goto stop;

    if (qp_add_raw_from_str(packer, "access") ||
        qp_add_array(&packer)) goto stop;

    for (vec_each(access, ti_auth_t, auth))
    {
        if (qp_add_array(&packer) ||
            qp_add_int64(packer, (int64_t) auth->user->id) ||
            qp_add_int64(packer, (int64_t) auth->mask) ||
            qp_close_array(packer)) goto stop;
    }

    if (qp_close_array(packer) || qp_close_map(packer)) goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc)
        log_error("failed to write file: `%s`", fn);
    qp_packer_destroy(packer);
    return rc;
}

int ti_access_restore(vec_t ** access, const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    unsigned char * data = fx_read(fn, &n);
    if (!data) return -1;

    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, data, (size_t) n);

    qp_res_t * res = qp_unpacker_res(&unpacker, &rcode);
    free(data);

    if (rcode)
    {
        log_critical(qp_strerror(rcode));
        return -1;
    }

    qp_res_t * schema, * qaccess;

    if (res->tp != QP_RES_MAP ||
        !(schema = qpx_map_get(res->via.map, "schema")) ||
        !(qaccess = qpx_map_get(res->via.map, "access")) ||
        schema->tp != QP_RES_INT64 ||
        schema->via.int64 != ti__access_fn_schema ||
        qaccess->tp != QP_RES_ARRAY) goto stop;

    for (uint32_t i = 0; i < qaccess->via.array->n; i++)
    {
        ti_user_t * user;
        qp_res_t * qauth = qaccess->via.array->values + i;
        qp_res_t * user_id, * mask;
        if (qauth->tp != QP_RES_ARRAY ||
            qauth->via.array->n != 2 ||
            !(user_id = qauth->via.array->values) ||
            !(mask = qauth->via.array->values + 1) ||
            user_id->tp != QP_RES_INT64 ||
            mask->tp != QP_RES_INT64) goto stop;

        user = thingsdb_users_get_by_id((uint64_t) user_id->via.int64);
        if (!user)
        {
            log_critical("missing user id: %"PRId64, user_id->via.int64);
            goto stop;
        }

        if (ti_access_grant(access, user, (uint64_t) mask->via.int64))
        {
            goto stop;
        }
    }

    rc = 0;

stop:
    if (rc) log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
