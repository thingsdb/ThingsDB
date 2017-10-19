/*
 * access.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/access.h>
#include <rql/auth.h>

int rql_access_grant(vec_t ** access, rql_user_t * user, uint64_t mask)
{
    for (vec_each(*access, rql_auth_t, auth))
    {
        if (auth->user == user)
        {
            auth->mask |= mask;
            return 0;
        }
    }
    rql_auth_t * auth = rql_auth_new(user, mask);
    if (!auth) return -1;

    if (vec_push(access, auth))
    {
        free(auth);
        return -1;
    }

    return 0;
}

void rql_access_revoke(vec_t * access, rql_user_t * user, uint64_t mask)
{
    uint32_t i = 0;
    for (vec_each(access, rql_auth_t, auth), i++)
    {
        if (auth->user == user)
        {
            auth->mask &= ~mask;
            if (!auth->mask)
            {
                vec_remove(access, i);
            }
        }
    }
}

int rql_access_check(vec_t * access, rql_user_t * user, uint64_t mask)
{

}
