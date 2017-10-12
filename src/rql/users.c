/*
 * users.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/rql.h>
#include <rql/users.h>
#include <util/cryptx.h>

rql_user_t * rql_users_auth(
        vec_t * users,
        qp_obj_t * name,
        qp_obj_t * pass,
        ex_t * e)
{
    char passbuf[rql_max_pass];
    char pw[CRYPTX_SZ];

    if (name->len < rql_min_name || name->len >= rql_max_name)
    {
        ex_set(e, RQL_FRONT_AUTH_ERR, "invalid user");
        return NULL;
    }

    if (pass->len < rql_min_pass || pass->len >= rql_max_pass)
    {
        ex_set(e, RQL_FRONT_AUTH_ERR, "invalid password");
        return NULL;
    }

    for (vec_each(users, rql_user_t, user))
    {
        if (strncmp(user->name, name->via.raw, name->len) == 0)
        {
            memcpy(passbuf, pass->via.raw, pass->len);
            passbuf[pass->len] = '\0';

            cryptx(passbuf, user->pass, pw);
            if (strcmp(pw, user->pass))
            {
                ex_set(e, RQL_FRONT_AUTH_ERR, "incorrect password");
                return NULL;
            }
            return user;
        }
    }

    ex_set(e, RQL_FRONT_AUTH_ERR, "user not found");
    return NULL;
}
