/*
 * ti/store/access.c
 */
#include <ti.h>
#include <qpack.h>
#include <ti/access.h>
#include <ti/users.h>
#include <ti/auth.h>
#include <ti/store/access.h>
#include <util/qpx.h>
#include <util/fx.h>


int ti_store_access_store(const vec_t * access, const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer)
        return -1;

    if (qp_add_map(&packer))
        goto stop;

    if (qp_add_raw_from_str(packer, "access") ||
        qp_add_array(&packer))
        goto stop;

    for (vec_each(access, ti_auth_t, auth))
    {
        if (qp_add_array(&packer) ||
            qp_add_int(packer, auth->user->id) ||
            qp_add_int(packer, auth->mask) ||
            qp_close_array(packer)) goto stop;
    }

    if (qp_close_array(packer) || qp_close_map(packer))
        goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc)
        log_error("failed to write file: `%s`", fn);
    else
        log_debug("stored access to file: `%s`", fn);

    qp_packer_destroy(packer);

    return rc;
}

int ti_store_access_restore(vec_t ** access, const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, data, (size_t) n);

    qp_res_t * res = qp_unpacker_res(&unpacker, &rcode);
    free(data);

    if (rcode)
    {
        log_critical(qp_strerror(rcode));
        return -1;
    }

    qp_res_t * qaccess;

    if (res->tp != QP_RES_MAP ||
        !(qaccess = qpx_map_get(res->via.map, "access")) ||
        qaccess->tp != QP_RES_ARRAY)
        goto stop;

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
            mask->tp != QP_RES_INT64)
            goto stop;

        user = ti_users_get_by_id((uint64_t) user_id->via.int64);
        if (!user)
        {
            log_critical("missing user id: %"PRId64, user_id->via.int64);
            goto stop;
        }

        if (ti_access_grant(access, user, (uint64_t) mask->via.int64))
            goto stop;
    }

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
