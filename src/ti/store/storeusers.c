/*
 * ti/store/storeusers.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/store/storeusers.h>
#include <ti/users.h>
#include <ti/val.inline.h>
#include <ti/token.h>
#include <ti/whitelist.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <util/vec.h>


static int store__whitelist(vec_t * whitelist, msgpack_packer * pk)
{

    if (whitelist)
    {
        if (msgpack_pack_array(pk, whitelist->n))
            return -1;
        for (vec_each(whitelist, ti_val_t, v))
            if (ti_val(v)->to_store_pk(v, pk))
                return -1;
    }
    else
    {
        if (msgpack_pack_nil(pk))
            return -1;
    }
    return 0;
}

int ti_store_users_store(const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "users") ||
        msgpack_pack_array(&pk, ti.users->n)
    ) goto fail;

    for (vec_each(ti.users, ti_user_t, user))
    {
        if (msgpack_pack_array(&pk, 7) ||
            msgpack_pack_uint64(&pk, user->id) ||
            mp_pack_strn(&pk, user->name->data, user->name->n) ||
            (user->encpass
                    ? mp_pack_str(&pk, user->encpass)
                    : msgpack_pack_nil(&pk)
            ) ||
            msgpack_pack_uint64(&pk, user->created_at) ||
            msgpack_pack_array(&pk, user->tokens->n)
        ) goto fail;

        for (vec_each(user->tokens, ti_token_t, token))
        {
            if (msgpack_pack_array(&pk, 4) ||
                mp_pack_strn(&pk, token->key, sizeof(ti_token_key_t)) ||
                msgpack_pack_uint64(&pk, token->expire_ts) ||
                msgpack_pack_uint64(&pk, token->created_at) ||
                mp_pack_str(&pk, token->description)
            ) goto fail;
        }

        if (store__whitelist(user->whitelists[TI_WHITELIST_ROOMS], &pk) ||
            store__whitelist(user->whitelists[TI_WHITELIST_PROCEDURES], &pk))
            goto fail;
    }

    log_debug("stored users to file: `%s`", fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", fn);
done:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        return -1;
    }
    (void) ti_sleep(5);
    return 0;
}

int ti_store_users_restore(const char * fn)
{
    ex_t e = {0};
    int rc = -1;
    size_t i, ii;
    ssize_t n;
    mp_obj_t arr, obj, mp_id, mp_name, mp_pass, mp_key,
             mp_expire, mp_desc, mp_created;
    mp_unp_t up;
    ti_token_t * token;
    ti_user_t * user;
    uchar * data = fx_read(fn, &n);
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = &up,
    };
    char * encrypted;
    if (!data || ti_users_clear())
        return -1;

    mp_unp_init(&up, data, (size_t) n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail;

    for (i = obj.via.sz; i--;)
    {
        /* TODO (COMPAT) Size 5 for compatibility with < 1.7.0 */
        if (mp_next(&up, &arr) != MP_ARR || (arr.via.sz != 5 && arr.via.sz != 7) ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_next(&up, &mp_pass) <= MP_END ||
            mp_next(&up, &mp_created) != MP_U64 ||
            mp_next(&up, &obj) != MP_ARR
        ) goto fail;

        encrypted = mp_pass.tp == MP_STR ? mp_strdup(&mp_pass) : NULL;

        user = ti_users_load_user(
                mp_id.via.u64,
                mp_name.via.str.data,
                mp_name.via.str.n,
                encrypted,
                mp_created.via.u64,
                &e);
        free(encrypted);

        if (!user)
        {
            log_critical(e.msg);
            goto fail;
        }

        for (ii = obj.via.sz; ii--;)
        {
            if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 4 ||
                mp_next(&up, &mp_key) != MP_STR ||
                mp_next(&up, &mp_expire) != MP_U64 ||
                mp_next(&up, &mp_created) != MP_U64 ||
                mp_next(&up, &mp_desc) != MP_STR
            ) goto fail;

            if (mp_key.via.str.n != sizeof(ti_token_key_t))
                goto fail;

            token = ti_token_create(
                    (ti_token_key_t *) mp_key.via.str.data,
                    mp_expire.via.u64,
                    mp_created.via.u64,
                    mp_desc.via.str.data,
                    mp_desc.via.str.n);

            if (!token || ti_user_add_token(user, token))
            {
                log_critical("failed to load token");
                ti_token_destroy(token);
                goto fail;
            }
        }

        /* TODO (COMPAT) Size used to be 5 prior to v1.7.0 */
        if (arr.via.sz == 7)
        {
            int wid;
            for (wid = 0; wid < 2; wid++)
            {
                if (mp_next(&up, &obj) == MP_ARR)
                {
                    user->whitelists[wid] = vec_new(obj.via.sz);
                    if (!user->whitelists[wid])
                        goto fail;
                    for (ii = obj.via.sz; ii--;)
                    {
                        ti_val_t * val = ti_val_from_vup(&vup);
                        if (!val)
                            goto fail;
                        VEC_push(user->whitelists[wid], val);
                    }
                }
            }
        }
    }

    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}
