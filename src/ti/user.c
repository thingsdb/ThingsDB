/*
 * user.c
 */
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/auth.h>
#include <ti/user.h>
#include <util/cryptx.h>
#include <util/strx.h>
#include <util/util.h>
#include <util/iso8601.h>

const char * ti_user_def_name = "admin";
const char * ti_user_def_pass = "pass";
const unsigned int ti_min_name = 1;
const unsigned int ti_max_name = 128;  /* max name = 127 + \0 */
const unsigned int ti_min_pass = 1;
const unsigned int ti_max_pass = 128;  /* max pass = 127 + \0 */

static int user__pack_access(
        ti_user_t * user,
        qp_packer_t ** packer,
        vec_t * access_,
        const unsigned char * scope,
        size_t n)
{
    for (vec_each(access_, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            if (qp_add_map(packer) ||
                qp_add_raw_from_str(*packer, "scope") ||
                qp_add_raw(*packer, scope, n) ||
                qp_add_raw_from_str(*packer, "privileges") ||
                qp_add_raw_from_str(
                        *packer,
                        ti_auth_mask_to_str(auth->mask)) ||
                qp_close_map(*packer))
                return -1;
            break;
        }
    }
    return 0;
}

static int user__pack_tokens(ti_user_t * user, qp_packer_t ** packer)
{
    uint64_t now = util_now_tsec();
    const size_t key_sz = sizeof(ti_token_key_t);

    if (qp_add_raw_from_str(*packer, "tokens") ||
        qp_add_array(packer))
        return -1;

    for (vec_each(user->tokens, ti_token_t, token))
    {
        const char * status, * expires_at;
        if (token->expire_ts)
        {
            status = token->expire_ts > now ? "OK" : "EXPIRED";
            expires_at = iso8601_time_str((const time_t *) &token->expire_ts);
        }
        else
        {
            status = "OK";
            expires_at = "never";
        }

        if (qp_add_map(packer) ||
            qp_add_raw_from_str(*packer, "key") ||
            qp_add_raw(*packer, (const uchar *) token->key, key_sz) ||
            qp_add_raw_from_str(*packer, "status") ||
            qp_add_raw_from_str(*packer, status) ||
            qp_add_raw_from_str(*packer, "expiration_time") ||
            qp_add_raw_from_str(*packer, expires_at) ||
            (*token->description && (
                qp_add_raw_from_str(*packer, "description") ||
                qp_add_raw_from_str(*packer, token->description)
            )) ||
            qp_close_map(*packer))
            return -1;
    }

    return qp_close_array(*packer);
}

ti_user_t * ti_user_create(
        uint64_t id,
        const char * name,
        size_t name_n,
        const char * encrpass)
{
    ti_user_t * user = malloc(sizeof(ti_user_t));
    if (!user)
        return NULL;

    user->id = id;
    user->ref = 1;
    user->name = ti_raw_create((uchar *) name, name_n);
    user->encpass = encrpass ? strdup(encrpass) : NULL;
    user->tokens = vec_new(0);

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
        ex_set(e, EX_BAD_DATA, "username must be between %u and %u characters",
                ti_min_name,
                ti_max_name);
        return false;
    }

    if (!strx_is_graphn(name, n))
    {
        ex_set(e, EX_BAD_DATA,
                "username should should only contain graphical characters");
        return false;
    }

    if (ti_users_get_by_namestrn(name, n))
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` already exists", (int) n, name);
        return false;
    }

    return true;
}

_Bool ti_user_pass_check(const char * passstr, ex_t * e)
{
    size_t n = strlen(passstr);
    if (n < ti_min_pass)
    {
        ex_set(e, EX_BAD_DATA, "password should be at least %u characters",
                ti_min_pass);
        return false;
    }

    if (n >= ti_max_pass)
    {
        ex_set(e, EX_BAD_DATA, "password should be less than %u characters",
                ti_max_pass);
        return false;
    }

    if (!strx_is_printable(passstr))
    {
        ex_set(e, EX_BAD_DATA,
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

    ti_val_drop((ti_val_t *) user->name);
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

int ti_user_info_to_packer(ti_user_t * user, qp_packer_t ** packer)
{
    if (qp_add_map(packer) ||
        qp_add_raw_from_str(*packer, "user_id") ||
        qp_add_int(*packer, user->id) ||
        qp_add_raw_from_str(*packer, "name") ||
        qp_add_raw(*packer, user->name->data, user->name->n) ||
        qp_add_raw_from_str(*packer, "has_password") ||
        qp_add_bool(*packer, !!user->encpass) ||
        qp_add_raw_from_str(*packer, "access") ||
        qp_add_array(packer))
        return -1;

    if (user__pack_access(
            user, packer, ti()->access_node, (uchar *) "@node", 5))
        return -1;

    if (user__pack_access(
            user, packer, ti()->access_thingsdb, (uchar *) "@thingsdb", 9))
        return -1;


    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        if (user__pack_access(
                user,
                packer,
                collection->access,
                collection->name->data,
                collection->name->n))
            return -1;
    }

    if (qp_close_array(*packer))
        return -1;

    if (user__pack_tokens(user, packer))
        return -1;

    return qp_close_map(*packer);
}

ti_val_t * ti_user_info_as_qpval(ti_user_t * user)
{
    ti_raw_t * ruser = NULL;
    qp_packer_t * packer = qp_packer_create2(256, 3);
    if (!packer)
        return NULL;

    if (ti_user_info_to_packer(user, &packer))
        goto fail;

    ruser = ti_raw_from_packer(packer);

fail:
    qp_packer_destroy(packer);
    return (ti_val_t * ) ruser;
}

ti_token_t * ti_user_pop_token_by_key(ti_user_t * user, ti_token_key_t * key)
{
    size_t idx = 0;
    for (vec_each(user->tokens, ti_token_t, token), ++idx)
        if (memcmp(token->key, key, sizeof(ti_token_key_t)) == 0)
            return vec_swap_remove(user->tokens, idx);
    return NULL;
}

