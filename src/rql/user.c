/*
 * user.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/rql.h>
#include <util/cryptx.h>

const char * rql_user_def_name = "iris";
const char * rql_user_def_pass = "siri";
const unsigned int rql_min_name = 1;
const unsigned int rql_max_name = 128;
const unsigned int rql_min_pass = 4;
const unsigned int rql_max_pass = 128;

rql_user_t * rql_user_create(
        uint64_t id,
        const char * name,
        const char * encrpass)
{
    rql_user_t * user = (rql_user_t *) malloc(sizeof(rql_user_t));
    if (!user) return NULL;

    user->id = id;
    user->ref = 1;
    user->name = strdup(name);
    user->pass = strdup(encrpass);

    if (!user->name || !user->pass)
    {
        rql_user_drop(user);
        return NULL;
    }

    return user;
}

rql_user_t * rql_user_grab(rql_user_t * user)
{
    user->ref++;
    return user;
}

void rql_user_drop(rql_user_t * user)
{
    if (user && !--user->ref)
    {
        free(user->pass);
        free(user->name);
        free(user);
    }
}

int rql_user_name_check(const char * name, ex_t * e)
{
    size_t n = strlen(name);
    if (n < rql_min_name)
    {
        ex_set(e, -1, "user name should be at least %u characters",
                rql_min_name);
        return -1;
    }

    if (n >= rql_max_name)
    {
        ex_set(e, -1, "user name should be less than %u characters",
                rql_max_name);
        return -1;
    }

    if (!strx_is_graph(name))
    {
        ex_set(e, -1, "user name should consist only of graphical characters");
        return -1;
    }
    return 0;
}

int rql_user_pass_check(const char * pass, ex_t * e)
{
    if (strlen(pass) < rql_min_pass)
    {
        ex_set(e, -1, "password should be at least %u characters",
                rql_min_pass);
        return -1;
    }


    if (strlen(pass) >= rql_max_pass)
    {
        ex_set(e, -1, "password should be less than %u characters",
                rql_max_pass);
        return -1;
    }
    return 0;
}

int rql_user_rename(rql_user_t * user, const char * name)
{
    char * username = strdup(name);
    if (!username) return -1;
    free(user->name);
    user->name = username;
    return 0;
}

int rql_user_set_pass(rql_user_t * user, const char * pass)
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
