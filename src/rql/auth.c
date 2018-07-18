/*
 * auth.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/auth.h>

rql_auth_t * rql_auth_new(rql_user_t * user, uint64_t mask)
{
    rql_auth_t * auth = malloc(sizeof(rql_auth_t));
    if (!auth)
        return NULL;
    auth->user = user;
    auth->mask = mask;
    return auth;
}
