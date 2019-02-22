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

const char * ti_user_def_name = "admin";
const char * ti_user_def_pass = "pass";
const unsigned int ti_min_name = 1;
const unsigned int ti_max_name = 128;
const unsigned int ti_min_pass = 1;
const unsigned int ti_max_pass = 128;

ti_user_t * ti_user_create(
        uint64_t id,
        const char * name,
        size_t n,
        const char * encrpass)
{
    ti_user_t * user = malloc(sizeof(ti_user_t));
    if (!user)
        return NULL;

    user->id = id;
    user->ref = 1;
    user->name = ti_raw_create((uchar *) name, n);
    user->encpass = strdup(encrpass);

    if (!user->name || !user->encpass)
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
        free(user);
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

    if (!ti_name_is_valid_strn(name, n))
    {
        ex_set(e, EX_BAD_DATA,
                "username should be a valid name, "
                "see "TI_DOCS"#names");
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

    if (!strx_is_graph(passstr))
    {
        ex_set(e, EX_BAD_DATA,
                "password should only contain graphical characters");
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

int ti_user_to_packer(ti_user_t * user, qp_packer_t ** packer)
{
    if (qp_add_map(packer) ||
        qp_add_raw_from_str(*packer, "user_id") ||
        qp_add_int(*packer, user->id) ||
        qp_add_raw_from_str(*packer, "name") ||
        qp_add_raw(*packer, user->name->data, user->name->n) ||
        qp_add_raw_from_str(*packer, "access") ||
        qp_add_array(packer))
        return -1;

    for (vec_each(ti()->access, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            if (qp_add_map(packer) ||
                qp_add_raw_from_str(*packer, "target") ||
                qp_add_int(*packer, 0) ||
                qp_add_raw_from_str(*packer, "privileges") ||
                qp_add_raw_from_str(
                        *packer,
                        ti_auth_mask_to_str(auth->mask)) ||
                qp_close_map(*packer))
                return -1;
            break;
        }
    }
    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        for (vec_each(collection->access, ti_auth_t, auth))
        {
            if (auth->user == user)
            {
                if (qp_add_map(packer) ||
                    qp_add_raw_from_str(*packer, "target") ||
                    qp_add_raw(
                            *packer,
                            collection->name->data,
                            collection->name->n) ||
                    qp_add_raw_from_str(*packer, "privileges") ||
                    qp_add_raw_from_str(
                            *packer,
                            ti_auth_mask_to_str(auth->mask)) ||
                    qp_close_map(*packer))
                    return -1;
                break;
            }
        }
    }

    return -(qp_close_array(*packer) || qp_close_map(*packer));
}

ti_val_t * ti_user_as_qpval(ti_user_t * user)
{
    ti_raw_t * ruser = NULL;
    qp_packer_t * packer = qp_packer_create2(256, 3);
    if (!packer)
        return NULL;

    if (ti_user_to_packer(user, &packer))
        goto fail;

    ruser = ti_raw_from_packer(packer);

fail:
    qp_packer_destroy(packer);
    return (ti_val_t * ) ruser;
}
