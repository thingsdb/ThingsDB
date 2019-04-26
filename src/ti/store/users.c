/*
 * ti/store/users.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/store/users.h>
#include <ti/users.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/vec.h>


int ti_store_users_store(const char * fn)
{
    ti_users_t * users = ti()->users;
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer)
        return -1;

    if (qp_add_map(&packer))
        goto stop;

    if (qp_add_raw_from_str(packer, "users") ||
        qp_add_array(&packer))
        goto stop;

    for (vec_each(users->vec, ti_user_t, user))
    {
        if (qp_add_array(&packer) ||
            qp_add_int(packer, user->id) ||
            qp_add_raw(packer, user->name->data, user->name->n) ||
            qp_add_raw_from_str(packer, user->encpass) ||
            qp_close_array(packer))
            goto stop;
    }

    if (qp_close_array(packer) || qp_close_map(packer))
        goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc)
        log_error("failed to write file: `%s`", fn);
    qp_packer_destroy(packer);
    return rc;
}

int ti_store_users_restore(const char * fn)
{
    ex_t * e = ex_use();
    int rcode, rc = -1;
    ssize_t n;
    qp_unpacker_t unpacker;
    qp_res_t * res;
    uchar * data = fx_read(fn, &n);
    qp_res_t * qusers;

    if (!data)
        return -1;

    ti_users_clear();

    qpx_unpacker_init(&unpacker, data, (size_t) n);
    res = qp_unpacker_res(&unpacker, &rcode);
    free(data);

    if (rcode)
    {
        log_critical(qp_strerror(rcode));
        return -1;
    }

    if (res->tp != QP_RES_MAP ||
        !(qusers = qpx_map_get(res->via.map, "users")) ||
        qusers->tp != QP_RES_ARRAY)
        goto stop;

    for (uint32_t i = 0; i < qusers->via.array->n; i++)
    {
        qp_res_t * quser = qusers->via.array->values + i;
        qp_res_t * qid, * qname, * qpass;
        char * encrypted, * name;
        uint64_t user_id, namelen;
        ti_user_t * user;

        if (quser->tp != QP_RES_ARRAY ||
                quser->via.array->n != 3 ||
            !(qid = quser->via.array->values) ||
            !(qname = quser->via.array->values + 1) ||
            !(qpass = quser->via.array->values + 2) ||
            qid->tp != QP_RES_INT64 ||
            qname->tp != QP_RES_RAW ||
            qpass->tp != QP_RES_RAW)
        {
            goto stop;
        }

        user_id = (uint64_t) qid->via.int64;
        name = (char *) qname->via.raw->data;
        namelen = qname->via.raw->n;
        encrypted = qpx_raw_to_str(qpass->via.raw);
        if (!encrypted)
            goto stop;

        user = ti_users_load_user(user_id, name, namelen, encrypted, e);
        free(encrypted);

        if (!user)
        {
            log_critical(e->msg);
            goto stop;
        }
    }

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
