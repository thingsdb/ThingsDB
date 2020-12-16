/*
 * user.c
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/auth.h>
#include <ti/raw.inline.h>
#include <ti/token.h>
#include <ti/user.h>
#include <ti/val.inline.h>
#include <util/cryptx.h>
#include <util/iso8601.h>
#include <util/mpack.h>
#include <util/strx.h>
#include <util/util.h>

const char * ti_user_def_name = "admin";
const char * ti_user_def_pass = "pass";
const unsigned int ti_min_name = 1;
const unsigned int ti_max_name = 128;  /* max name = 127 + \0 */
const unsigned int ti_min_pass = 1;
const unsigned int ti_max_pass = 128;  /* max pass = 127 + \0 */

static int user__pack_access(
        ti_user_t * user,
        msgpack_packer * pk,
        vec_t * access_,
        const unsigned char * scope,
        size_t n)
{
    for (vec_each(access_, ti_auth_t, auth))
        if (auth->user == user)
            return (
                msgpack_pack_map(pk, 2) ||

                mp_pack_str(pk, "scope") ||
                (*scope == '@'
                    ? mp_pack_strn(pk, scope, n)
                    : mp_pack_fmt(pk, "@collection:%.*s", (int) n, scope)) ||

                mp_pack_str(pk, "privileges") ||
                mp_pack_str(pk, ti_auth_mask_to_str(auth->mask))
            ) ? -1 : 0;
    return 0;
}

static int user__pack_tokens(ti_user_t * user, msgpack_packer * pk)
{
    uint64_t now = util_now_tsec();
    const size_t key_sz = sizeof(ti_token_key_t);

    if (mp_pack_str(pk, "tokens") ||
        msgpack_pack_array(pk, user->tokens->n)
    ) return -1;

    for (vec_each(user->tokens, ti_token_t, token))
    {
        const char * status, * isotimestr;

        if (token->expire_ts)
        {
            status = token->expire_ts > now ? "OK" : "EXPIRED";
            isotimestr = ti_datetime_ts_str((const time_t *) &token->expire_ts);
        }
        else
        {
            status = "OK";
            isotimestr = "never";
        }

        if (msgpack_pack_map(pk, *token->description ? 5 : 4) ||

            mp_pack_str(pk, "key") ||
            mp_pack_strn(pk, token->key, key_sz) ||

            mp_pack_str(pk, "status") ||
            mp_pack_str(pk, status) ||

            mp_pack_str(pk, "expiration_time") ||
            mp_pack_str(pk, isotimestr) ||

            (*token->description && (
                mp_pack_str(pk, "description") ||
                mp_pack_str(pk, token->description)
            ))
        ) return -1;

        /* the iso8601_time_str() return a pointer to an internal buffer so
         * we cannot have both the created and expire time together
         */
        isotimestr = ti_datetime_ts_str((const time_t *) &token->created_at);

        if (mp_pack_str(pk, "created_on") || mp_pack_str(pk, isotimestr))
            return -1;
    }

    return 0;
}

ti_user_t * ti_user_create(
        uint64_t id,
        const char * name,
        size_t name_n,
        const char * encrpass,
        uint64_t created_at)
{
    ti_user_t * user = malloc(sizeof(ti_user_t));
    if (!user)
        return NULL;

    user->id = id;
    user->ref = 1;
    user->name = ti_str_create(name, name_n);
    user->encpass = encrpass ? strdup(encrpass) : NULL;
    user->tokens = vec_new(0);
    user->created_at = created_at;

    if (!user->name || (encrpass && !user->encpass) || !user->tokens)
    {
        ti_user_drop(user);
        return NULL;
    }

    return user;
}

void ti_user_drop(ti_user_t * user)
{
    if (user && !--user->ref)
    {
        free(user->encpass);
        ti_val_drop((ti_val_t *) user->name);
        vec_destroy(user->tokens, (vec_destroy_cb) ti_token_destroy);
        free(user);
    }
}

void ti_user_del_expired(ti_user_t * user, uint64_t after_ts)
{
    ti_token_t * token;
    for (size_t n = 0, m = user->tokens->n; n < m; )
    {
        token = user->tokens->data[n];
        if (token->expire_ts && token->expire_ts < after_ts)
        {
            ti_token_destroy(vec_swap_remove(user->tokens, n));
            --m;
            continue;
        }
        ++n;
    }
}

_Bool ti_user_name_check(const char * name, size_t n, ex_t * e)
{
    if (n < ti_min_name || n >= ti_max_name)
    {
        ex_set(e, EX_VALUE_ERROR, "username must be between %u and %u characters",
                ti_min_name,
                ti_max_name);
        return false;
    }

    if (!strx_is_graphn(name, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "username should should only contain graphical characters");
        return false;
    }

    if (ti_users_get_by_namestrn(name, n))
    {
        ex_set(e, EX_LOOKUP_ERROR, "user `%.*s` already exists", (int) n, name);
        return false;
    }

    return true;
}

_Bool ti_user_pass_check(const char * passstr, ex_t * e)
{
    size_t n = strlen(passstr);
    if (n < ti_min_pass)
    {
        ex_set(e, EX_VALUE_ERROR, "password should be at least %u characters",
                ti_min_pass);
        return false;
    }

    if (n >= ti_max_pass)
    {
        ex_set(e, EX_VALUE_ERROR, "password should be less than %u characters",
                ti_max_pass);
        return false;
    }

    if (!strx_is_printable(passstr))
    {
        ex_set(e, EX_VALUE_ERROR,
                "password should only contain printable characters");
        return false;
    }
    return true;
}

/*
 * Increments the `name` reference counter only if successful
 */
int ti_user_rename(ti_user_t * user, ti_raw_t * name, ex_t * e)
{
    assert (e->nr == 0);

    if (!ti_user_name_check((const char *) name->data, name->n, e))
        return e->nr;

    ti_val_unsafe_drop((ti_val_t *) user->name);
    user->name = ti_grab(name);

    return e->nr;
}

int ti_user_set_pass(ti_user_t * user, const char * pass)
{
    char * password;
    char salt[CRYPTX_SALT_SZ];
    char encrypted[CRYPTX_SZ];

    if (!pass)
    {
        /* clear password */
        free(user->encpass);
        user->encpass = NULL;
        return 0;
    }

    /* generate a random salt */
    cryptx_gen_salt(salt);

    /* encrypt the users password */
    cryptx(pass, salt, encrypted);

    /* replace user password with encrypted password */
    password = strdup(encrypted);
    if (!password)
        return -1;

    /* free the old password */
    free(user->encpass);
    user->encpass = password;

    return 0;
}

size_t user__count_access(ti_user_t * user)
{
    size_t n = 0;
    for (vec_each(ti.access_node, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            ++n;
            break;
        }
    }

    for (vec_each(ti.access_thingsdb, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            ++n;
            break;
        }
    }

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        for (vec_each(collection->access, ti_auth_t, auth))
        {
            if (auth->user == user)
            {
                ++n;
                break;
            }
        }
    }
    return n;
}

int ti_user_info_to_pk(ti_user_t * user, msgpack_packer * pk)
{
    if (msgpack_pack_map(pk, 6) ||  /* four below + tokens */

        mp_pack_str(pk, "user_id") ||
        msgpack_pack_uint64(pk, user->id) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, user->name->data, user->name->n) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, user->created_at) ||

        mp_pack_str(pk, "has_password") ||
        mp_pack_bool(pk, !!user->encpass) ||

        mp_pack_str(pk, "access") ||
        msgpack_pack_array(pk, user__count_access(user))
    ) return -1;

    if (user__pack_access(
            user, pk, ti.access_node, (uchar *) "@node", 5))
        return -1;

    if (user__pack_access(
            user, pk, ti.access_thingsdb, (uchar *) "@thingsdb", 9))
        return -1;

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        if (user__pack_access(
                user,
                pk,
                collection->access,
                collection->name->data,
                collection->name->n))
            return -1;
    }

    if (user__pack_tokens(user, pk))
        return -1;

    return 0;
}

ti_val_t * ti_user_as_mpval(ti_user_t * user)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_user_info_to_pk(user, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}

ti_token_t * ti_user_pop_token_by_key(ti_user_t * user, ti_token_key_t * key)
{
    size_t idx = 0;
    for (vec_each(user->tokens, ti_token_t, token), ++idx)
        if (memcmp(token->key, key, sizeof(ti_token_key_t)) == 0)
            return vec_swap_remove(user->tokens, idx);
    return NULL;
}
