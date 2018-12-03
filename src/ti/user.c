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

const char * ti_user_def_name = "iris";
const char * ti_user_def_pass = "siri";
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
        LOGC("USER DROPPED: %.*s",
                (int) user->name->n, (char *) user->name->data);
        free(user->encpass);
        ti_raw_drop(user->name);
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

int ti_user_rename(ti_user_t * user, ti_raw_t * name)
{
    ti_raw_t * username = ti_grab(name);
    if (!username)
        return -1;
    /* free the old user name */
    free(user->name);
    user->name = username;
    return 0;
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

ti_val_t * ti_user_as_qpval(ti_user_t * user)
{
    ti_val_t * qpval;
    ti_raw_t * ruser;
    qp_packer_t * packer = qp_packer_create2(128, 3);
    if (!packer)
        return NULL;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int64(packer, user->id);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw(packer, user->name->data, user->name->n);
    (void) qp_add_raw_from_str(packer, "access");
    (void) qp_add_array(&packer);
    for (vec_each(ti()->access, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            if (qp_add_map(&packer) ||
                qp_add_raw_from_str(packer, "target") ||
                qp_add_int64(packer, 0) ||
                qp_add_raw_from_str(packer, "privileges") ||
                qp_add_raw_from_str(
                        packer,
                        ti_auth_mask_to_str(auth->mask)) ||
                qp_close_map(packer))
                goto fail0;
            break;
        }
    }
    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        for (vec_each(collection->access, ti_auth_t, auth))
        {
            if (auth->user == user)
            {
                if (qp_add_map(&packer) ||
                    qp_add_raw_from_str(packer, "target") ||
                    qp_add_raw(
                            packer,
                            collection->name->data,
                            collection->name->n) ||
                    qp_add_raw_from_str(packer, "privileges") ||
                    qp_add_raw_from_str(
                            packer,
                            ti_auth_mask_to_str(auth->mask)) ||
                    qp_close_map(packer))
                    goto fail0;
                break;
            }
        }
    }

    if (qp_close_array(packer) || qp_close_map(packer))
        goto fail0;

    ruser = ti_raw_from_packer(packer);
    if (!ruser)
        goto fail0;

    qp_packer_destroy(packer);

    qpval = ti_val_weak_create(TI_VAL_QP, ruser);
    if (!qpval)
        goto fail1;

    return qpval;

fail1:
    ti_raw_drop(ruser);
fail0:
    qp_packer_destroy(packer);
    return NULL;
}
