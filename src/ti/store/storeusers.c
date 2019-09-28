/*
 * ti/store/users.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti/store/storeusers.h>
#include <ti.h>
#include <ti/users.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/vec.h>


int ti_store_users_store(const char * fn)
{
    ti_users_t * users = ti()->users;
    int rc = -1;
    qp_packer_t * packer = qp_packer_create2(512 + (users->vec->n * 512), 5);
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
            (user->encpass
                    ? qp_add_raw_from_str(packer, user->encpass)
                    : qp_add_null(packer)) ||
            qp_add_array(&packer))
            goto stop;

        for (vec_each(user->tokens, ti_token_t, token))
        {
            if (qp_add_array(&packer) ||
                qp_add_raw(
                        packer,
                        (const uchar *) token->key,
                        sizeof(ti_token_key_t)) ||
                qp_add_int(packer, token->expire_ts) ||
                qp_add_raw_from_str(packer, token->description) ||
                qp_close_array(packer))
                goto stop;
        }

        if (qp_close_array(packer) || qp_close_array(packer))
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
    ex_t e = {0};
    int rcode, rc = -1;
    ssize_t n;
    qp_unpacker_t unpacker;
    qp_res_t * res;
    uchar * data = fx_read(fn, &n);
    qp_res_t * qusers;

    if (!data || ti_users_clear())
        return -1;

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
        qp_res_t * qid, * qname, * qpass, *qtokes;
        char * encrypted = NULL, * name;
        uint64_t user_id, namelen;
        ti_user_t * user;

        if (quser->tp != QP_RES_ARRAY ||
                quser->via.array->n != 4 ||
            !(qid = quser->via.array->values) ||
            !(qname = quser->via.array->values + 1) ||
            !(qpass = quser->via.array->values + 2) ||
            !(qtokes = quser->via.array->values + 3) ||
            qid->tp != QP_RES_INT64 ||
            qname->tp != QP_RES_RAW ||
            (qpass->tp != QP_RES_RAW && qpass->tp != QP_RES_NULL) ||
            qtokes->tp != QP_RES_ARRAY)
        {
            goto stop;
        }

        user_id = (uint64_t) qid->via.int64;
        name = (char *) qname->via.raw->data;
        namelen = qname->via.raw->n;
        if (qpass->tp == QP_RES_RAW)
        {
            encrypted = qpx_raw_to_str(qpass->via.raw);
            if (!encrypted)
                goto stop;
        }
        user = ti_users_load_user(user_id, name, namelen, encrypted, &e);
        free(encrypted);

        if (!user)
        {
            log_critical(e.msg);
            goto stop;
        }

        for (uint32_t t = 0; t < qtokes->via.array->n; t++)
        {
            ti_token_t * token;
            uint64_t expire_ts;
            qp_res_t * qtoken = qtokes->via.array->values + t;
            qp_res_t * qkey, * qexpire, * qdescription;
            if (qtoken->tp != QP_RES_ARRAY ||
                qtoken->via.array->n != 3 ||
                !(qkey = qtoken->via.array->values) ||
                !(qexpire = qtoken->via.array->values + 1) ||
                !(qdescription = qtoken->via.array->values + 2) ||
                (   qkey->tp != QP_RES_RAW ||
                    qkey->via.raw->n != sizeof(ti_token_key_t)) ||
                qexpire->tp != QP_RES_INT64 ||
                qdescription->tp != QP_RES_RAW)
            {
                goto stop;
            }

            expire_ts = (uint64_t) qexpire->via.int64;
            token = ti_token_create(
                    (ti_token_key_t *) qkey->via.raw->data,
                    expire_ts,
                    (const char *) qdescription->via.raw->data,
                    qdescription->via.raw->n);
            if (!token || ti_user_add_token(user, token))
            {
                ti_token_destroy(token);
                goto stop;
            }
        }
    }

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
