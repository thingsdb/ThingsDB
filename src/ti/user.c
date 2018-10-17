/*
 * user.c
 */
#include <stdlib.h>
#include <util/cryptx.h>
#include <util/strx.h>

const char * ti_user_def_name = "iris";
const char * ti_user_def_pass = "siri";
const unsigned int ti_min_name = 1;
const unsigned int ti_max_name = 128;
const unsigned int ti_min_pass = 4;
const unsigned int ti_max_pass = 128;

ti_user_t * ti_user_create(
        uint64_t id,
        const ti_raw_t * name,
        const char * encrpass)
{
    ti_user_t * user = (ti_user_t *) malloc(sizeof(ti_user_t));
    if (!user) return NULL;

    user->id = id;
    user->ref = 1;
    user->name = ti_raw_dup(name);
    user->pass = strdup(encrpass);

    if (!user->name || !user->pass)
    {
        ti_user_drop(user);
        return NULL;
    }

    return user;
}

ti_user_t * ti_user_grab(ti_user_t * user)
{
    user->ref++;
    return user;
}

void ti_user_drop(ti_user_t * user)
{
    if (user && !--user->ref)
    {
        free(user->pass);
        free(user->name);
        free(user);
    }
}

int ti_user_name_check(const ti_raw_t * name, ex_t * e)
{
    if (name->n < ti_min_name)
    {
        ex_set(e, -1, "user name should be at least %u characters",
                ti_min_name);
        return -1;
    }

    if (name->n >= ti_max_name)
    {
        ex_set(e, -1, "user name should be less than %u characters",
                ti_max_name);
        return -1;
    }

    if (!strx_is_graphn((const char *) name->data, name->n))
    {
        ex_set(e, -1, "user name should consist only of graphical characters");
        return -1;
    }
    return 0;
}

int ti_user_pass_check(const ti_raw_t * pass, ex_t * e)
{
    if (pass->n < ti_min_pass)
    {
        ex_set(e, -1, "password should be at least %u characters",
                ti_min_pass);
        return -1;
    }


    if (pass->n >= ti_max_pass)
    {
        ex_set(e, -1, "password should be less than %u characters",
                ti_max_pass);
        return -1;
    }
    return 0;
}

int ti_user_rename(ti_user_t * user, const ti_raw_t * name)
{
    ti_raw_t * username = ti_raw_dup(name);
    if (!username) return -1;
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
    if (!password) return -1;
    free(user->pass);
    user->pass = password;

    return 0;
}
