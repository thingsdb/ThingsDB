/*
 * ti/users.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/proto.h>
#include <ti/users.h>
#include <ti/token.h>
#include <util/cryptx.h>
#include <util/logger.h>
#include <util/util.h>
#include <util/vec.h>

static ti_users_t * users;
static ti_users_t users_;

int ti_users_create(void)
{
    users = &users_;

    users->vec = vec_new(1);
    if (!users->vec)
        goto failed;

    ti()->users = users;
    return 0;

failed:
    users = NULL;
    return -1;
}

void ti_users_destroy(void)
{
    if (!users)
        return;
    vec_destroy(users->vec, (vec_destroy_cb) ti_user_drop);
    ti()->users = users = NULL;
}

ti_user_t * ti_users_new_user(
        const char * name,
        size_t n,
        const char * passstr,
        ex_t * e)
{
    ti_user_t * user = NULL;
    assert (e->nr == 0);

    if (!ti_user_name_check(name, n, e))
        goto done;

    if (passstr && !ti_user_pass_check(passstr, e))
        goto done;

    user = ti_user_create(ti_next_thing_id(), name, n, NULL);

    if (!user ||
        ti_user_set_pass(user, passstr) ||
        vec_push(&users->vec, user))
    {
        ti_user_drop(user);
        user = NULL;
        ex_set_mem(e);
        goto done;
    }

done:
    return user;
}

ti_user_t * ti_users_load_user(
        uint64_t user_id,
        const char * name,
        size_t n,
        const char * encrypted,
        ex_t * e)
{
    ti_user_t * user = NULL;
    assert (e->nr == 0);

    if (!ti_user_name_check(name, n, e))
        goto done;

    if (ti_users_get_by_id(user_id))
    {
        ex_set(e, EX_LOOKUP_ERROR, "`user:%"PRIu64"` already exists", user_id);
        goto done;
    }

    if (user_id >= ti()->node->next_thing_id)
        ti()->node->next_thing_id = user_id + 1;

    user = ti_user_create(user_id, name, n, encrypted);

    if (!user || vec_push(&users->vec, user))
    {
        ti_user_drop(user);
        user = NULL;
        ex_set_mem(e);
        goto done;
    }

done:
    return user;
}

/* clears users and access */
int ti_users_clear(void)
{
    /* we need a copy since `del_user` will change the vec */
    vec_t * vec = vec_dup(users->vec);
    if (!vec)
        return -1;

    vec_destroy(vec, (vec_destroy_cb) ti_users_del_user);
    return 0;
}

void ti_users_del_user(ti_user_t * user)
{
    size_t i = 0;

    /* remove node access */
    ti_access_revoke(ti()->access_node, user, TI_AUTH_MASK_FULL);

    /* remove thingsdb access */
    ti_access_revoke(ti()->access_thingsdb, user, TI_AUTH_MASK_FULL);

    /* remove collection access */
    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        ti_access_revoke(collection->access, user, TI_AUTH_MASK_FULL);
    }

    /* remove user */
    for (vec_each(users->vec, ti_user_t, usr), ++i)
    {
        if (usr == user)
        {
            ti_user_drop(vec_swap_remove(users->vec, i));
            return;
        }
    }
}

/*
 * Both `name` and `pass` must have been checked to be of `raw` type.
 * Returns a `borrowed` user or NULL if not found and `e` is set.
 */
ti_user_t * ti_users_auth(mp_obj_t * mp_name, mp_obj_t * mp_pass, ex_t * e)
{
    assert (mp_name->tp == MP_STR);
    assert (mp_pass->tp == MP_STR);

    char passbuf[ti_max_pass];
    char pw[CRYPTX_SZ];

    if (mp_name->via.str.n < ti_min_name || mp_name->via.str.n >= ti_max_name)
        goto failed;

    if (mp_pass->via.str.n < ti_min_pass || mp_pass->via.str.n >= ti_max_pass)
        goto failed;

    for (vec_each(users->vec, ti_user_t, user))
    {
        if (ti_raw_eq_strn(
                user->name,
                mp_name->via.str.data,
                mp_name->via.str.n))
        {
            if (!user->encpass)
                goto failed;

            memcpy(passbuf, mp_pass->via.str.data, mp_pass->via.str.n);
            passbuf[mp_pass->via.str.n] = '\0';

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

/*
 * Returns a `borrowed` user or NULL if not found and `e` is set.
 */
ti_user_t * ti_users_auth_by_token(mp_obj_t * mp_token, ex_t * e)
{
    uint64_t now_ts = util_now_tsec();
    const size_t key_sz = sizeof(ti_token_key_t);

    if (mp_token->tp != MP_STR || mp_token->via.str.n != key_sz)
        goto invalid;

    for (vec_each(users->vec, ti_user_t, user))
    {
        for (vec_each(user->tokens, ti_token_t, token))
        {
            if (memcmp(token->key, mp_token->via.str.data, key_sz) == 0)
            {
                if (token->expire_ts && token->expire_ts < now_ts)
                    goto expired;
                return user;
            }
        }
    }
    /* bubble down to `invalid token` if not found */
invalid:
    ex_set(e, EX_AUTH_ERROR, "invalid token");
    return NULL;
expired:
    ex_set(e, EX_AUTH_ERROR, "token is expired"DOC_DEL_EXPIRED);
    return NULL;
}

/*
 * Returns a `borrowed` user or NULL if not found and `e` is set.
 */
ti_user_t * ti_users_auth_by_basic(const char * b64, size_t n, ex_t * e)
{
    ti_user_t * user;
    mp_obj_t mp_user, mp_pass;
    ti_raw_t * auth = ti_bytes_from_base64(b64, n);
    if (!auth)
    {
        ex_set_mem(e);
        return NULL;
    }

    mp_user.tp = MP_STR;
    mp_user.via.bin.data = auth->data;

    for (size_t n = 0, end = auth->n; n < end; ++n)
    {
        if (auth->data[n] == ':')
        {
            mp_user.via.bin.n = n;

            ++n;
            if (n > end)
                goto failed;

            mp_pass.tp = MP_STR;
            mp_pass.via.bin.data = auth->data + n;
            mp_pass.via.bin.n = end - n;

            user = ti_users_auth(&mp_user, &mp_pass, e);
            goto done;
        }
    }

failed:
    ex_set(e, EX_AUTH_ERROR, "invalid basic authentication");
done:
    ti_val_drop((ti_val_t *) auth);
    return user;
}

/* Returns a borrowed reference */
ti_user_t * ti_users_get_by_id(uint64_t id)
{
    for (vec_each(users->vec, ti_user_t, user))
        if (user->id == id)
            return user;
    return NULL;
}

/* Returns a borrowed reference */
ti_user_t * ti_users_get_by_namestrn(const char * name, size_t n)
{
    for (vec_each(users->vec, ti_user_t, user))
        if (ti_raw_eq_strn(user->name, name, n))
            return user;
    return NULL;
}

ti_varr_t * ti_users_info(void)
{
    vec_t * vec = users->vec;
    ti_varr_t * varr = ti_varr_create(vec->n);
    if (!varr)
        return NULL;

    for (vec_each(vec, ti_user_t, user))
    {
        ti_val_t * mpinfo = ti_user_as_mpval(user);
        if (!mpinfo)
        {
            ti_val_drop((ti_val_t *) varr);
            return NULL;
        }
        VEC_push(varr->vec, mpinfo);
    }
    return varr;
}

void ti_users_del_expired(uint64_t after_ts)
{
    for (vec_each(users->vec, ti_user_t, user))
        ti_user_del_expired(user, after_ts);
}

ti_token_t * ti_users_pop_token_by_key(ti_token_key_t * key)
{
    ti_token_t * token;
    for (vec_each(users->vec, ti_user_t, user))
        if ((token = ti_user_pop_token_by_key(user, key)))
            return token;
    return NULL;
}

