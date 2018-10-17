/*
 * auth.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <ti/auth.h>

ti_auth_t * ti_auth_new(ti_user_t * user, uint64_t mask)
{
    ti_auth_t * auth = malloc(sizeof(ti_auth_t));
    if (!auth)
        return NULL;
    auth->user = user;
    auth->mask = mask;
    return auth;
}
