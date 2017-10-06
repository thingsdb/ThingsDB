/*
 * user.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <rql/user.h>

rql_user_t * rql_user_create(const char * name)
{
    rql_user_t * user = (rql_user_t *) malloc(sizeof(rql_user_t));
    if (!user) return NULL;

    user->ref = 1;
    user->name = strdup(name);

    if (!user->name)
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
        free(user->name);
        free(user);
    }
}

int rql_user_rename(rql_user_t * user, const char * name)
{
    free(user->name);
    user->name = strdup(name);
    return (user->name) ? 0 : -1;
}
