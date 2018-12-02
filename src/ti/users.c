/*
 * users.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti/proto.h>
#include <ti/users.h>
#include <ti.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/qpx.h>
#include <util/logger.h>
#include <util/vec.h>

static vec_t ** users = NULL;

const int ti_users_fn_schema = 0;

int ti_users_create(void)
{
    ti()->users = vec_new(0);
    users = &ti()->users;
    return -(*users == NULL);
}

void ti_users_destroy(void)
{
    if (!users)
        return;
    vec_destroy(*users, (vec_destroy_cb) ti_user_drop);
    *users = NULL;
}

ti_user_t * ti_users_create_user(
        const char * name,
        size_t n,
        const char * passstr,
        ex_t * e)
{
    ti_user_t * user = NULL;
    assert (e->nr == 0);

    if (!ti_user_name_check(name, n, e))
        goto done;

    if (!ti_user_pass_check(passstr, e))
        goto done;

    if (ti_users_get_by_namestrn(name, n))
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` already exists", (int) n, name);
        goto done;
    }

    user = ti_user_create(ti_next_thing_id(), name, n, "");

    if (!user || ti_user_set_pass(user, passstr) || vec_push(users, user))
    {
        ti_user_drop(user);
        user = NULL;
        ex_set_alloc(e);
        goto done;
    }

done:
    return user;
}

ti_user_t * ti_users_auth(qp_obj_t * name, qp_obj_t * pass, ex_t * e)
{
    char passbuf[ti_max_pass];
    char pw[CRYPTX_SZ];

    if (name->len < ti_min_name || name->len >= ti_max_name)
        goto failed;

    if (pass->len < ti_min_pass || pass->len >= ti_max_pass)
        goto failed;

    for (vec_each(*users, ti_user_t, user))
    {
        if (qpx_obj_eq_raw(name, user->name))
        {
            memcpy(passbuf, pass->via.raw, pass->len);
            passbuf[pass->len] = '\0';

            cryptx(passbuf, user->encpass, pw);
            if (strcmp(pw, user->encpass))
                goto failed;
            return user;
        }
    }

failed:
    ex_set(e, EX_AUTH_ERROR, "invalid username or password");
    return NULL;
}

/* Returns a borrowed reference */
ti_user_t * ti_users_get_by_id(uint64_t id)
{
    for (vec_each(*users, ti_user_t, user))
        if (user->id == id)
            return user;
    return NULL;
}

/* Returns a borrowed reference */
ti_user_t * ti_users_get_by_namestrn(const char * name, size_t n)
{
    for (vec_each(*users, ti_user_t, user))
        if (ti_raw_equal_strn(user->name, name, n))
            return user;
    return NULL;
}

int ti_users_store(const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer) return -1;

    if (qp_add_map(&packer)) goto stop;

    /* schema */
    if (qp_add_raw_from_str(packer, "schema") ||
        qp_add_int64(packer, ti_users_fn_schema)) goto stop;

    if (qp_add_raw_from_str(packer, "users") ||
        qp_add_array(&packer)) goto stop;

    for (vec_each(*users, ti_user_t, user))
    {
        if (qp_add_array(&packer) ||
            qp_add_int64(packer, (int64_t) user->id) ||
            qp_add_raw(packer, user->name->data, user->name->n) ||
            qp_add_raw_from_str(packer, user->encpass) ||
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

int ti_users_restore(const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    uchar * data = fx_read(fn, &n);
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

    qp_res_t * schema, * qusers;

    if (res->tp != QP_RES_MAP ||
        !(schema = qpx_map_get(res->via.map, "schema")) ||
        !(qusers = qpx_map_get(res->via.map, "users")) ||
        schema->tp != QP_RES_INT64 ||
        schema->via.int64 != ti_users_fn_schema ||
        qusers->tp != QP_RES_ARRAY) goto stop;

    for (uint32_t i = 0; i < qusers->via.array->n; i++)
    {
        qp_res_t * quser = qusers->via.array->values + i;
        qp_res_t * qid, * qname, * qpass;
        ti_user_t * user;
        char * passstr, * name;
        uint64_t user_id, namelen;

        if (quser->tp != QP_RES_ARRAY ||
                quser->via.array->n != 3 ||
            !(qid = quser->via.array->values) ||
            !(qname = quser->via.array->values + 1) ||
            !(qpass = quser->via.array->values + 2) ||
            qid->tp != QP_RES_INT64 ||
            qname->tp != QP_RES_RAW ||
            qpass->tp != QP_RES_RAW) goto stop;

        user_id = (uint64_t) qid->via.int64;
        name = (char *) qname->via.raw->data;
        namelen = qname->via.raw->n;
        passstr = qpx_raw_to_str(qpass->via.raw);
        if (!passstr)
            goto stop;

        user = ti_user_create(user_id, name, namelen, passstr);

        free(passstr);

        if (!user || vec_push(users, user))
            goto stop;
    }

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
