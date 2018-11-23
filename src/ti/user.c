/*
 * user.c
 */
#include <stdlib.h>
#include <util/cryptx.h>
#include <util/strx.h>
#include <ti/user.h>
#include <ti.h>

const char * ti_user_def_name = "iris";
const char * ti_user_def_pass = "siri";
const unsigned int ti_min_name = 1;
const unsigned int ti_max_name = 128;
const unsigned int ti_min_pass = 4;
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
    user->qpdata = NULL;

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
        free(user->name);
        /* TODO: free  user->qpdata */
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
