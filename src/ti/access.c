/*
 * ti/access.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/users.h>
#include <util/logger.h>

int ti_access_grant(vec_t ** access, ti_user_t * user, uint64_t mask)
{
    ti_auth_t * auth;
    for (vec_each(*access, ti_auth_t, auth))
    {
        if (auth->user == user)
        {
            auth->mask |= mask;
            return 0;
        }
    }
    auth = ti_auth_create(user, mask);
    if (!auth || vec_push(access, auth))
    {
        ti_auth_destroy(auth);
        return -1;
    }

    return 0;
}

void ti_access_revoke(vec_t * access, ti_user_t * user, uint64_t mask)
{
    uint32_t i = 0;
    for (vec_each(access, ti_auth_t, auth), i++)
    {
        if (auth->user == user)
        {
            auth->mask &= ~mask;
            if (!auth->mask)
                ti_auth_destroy(vec_remove(access, i));
            return;
        }
    }
}

_Bool ti_access_check(const vec_t * access, ti_user_t * user, uint64_t mask)
{
    for (vec_each(access, ti_auth_t, auth))
        if (auth->user == user)
            return (auth->mask & mask) == mask;
    return false;
}

/* Return 0 if the user has the required privileges or EX_FORBIDDEN if not
 * with e->msg set
 */
int ti_access_check_err(
        const vec_t * access,
        ti_user_t * user,
        uint64_t mask,
        ex_t * e)
{
    assert (e->nr == 0);
    if (!ti_access_check(access, user, mask))
        ex_set(e, EX_FORBIDDEN,
                "user `%.*s` is missing the required privileges (`%s`)",
                (int) user->name->n, (char *) user->name->data,
                ti_auth_mask_to_str(TI_AUTH_READ));
    return e->nr;
}



