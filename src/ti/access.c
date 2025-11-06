/*
 * ti/access.c
 */
#include <assert.h>
#include <doc.h>
#include <ex.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/users.h>
#include <util/logger.h>

/*
 * Helper function to add more info to an authentication (forbidden) error.
 */
static void access__helper_str(
        const vec_t * access,
        char ** scope,
        char ** name,
        size_t * name_sz)
{
    if (access == ti.access_thingsdb)
    {
        *scope = "@thingsdb";
        *name = "";
        *name_sz = 0;
        return;
    }

    if (access == ti.access_node)
    {
        *scope = "@node";
        *name = "";
        *name_sz = 0;;
        return;
    }

    *scope = "@collection:";

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        if (access == collection->access)
        {
            *name = (char *) collection->name->data;
            *name_sz = collection->name->n;
            return;
        }
    }

    *name = "<unknown>";
    *name_sz = strlen("<unknown>");
}

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
                ti_auth_destroy(vec_swap_remove(access, i));
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

_Bool ti_access_check_or(const vec_t * access, ti_user_t * user, uint64_t mask)
{
    for (vec_each(access, ti_auth_t, auth))
        if (auth->user == user)
            return auth->mask & mask;
    return false;
}


/*
 * Return 0 if the user has the required privileges or EX_FORBIDDEN if not
 * with e->msg set
 */
int ti_access_check_err(
        const vec_t * access,
        ti_user_t * user,
        uint64_t mask,
        ex_t * e)
{
    assert(e->nr == 0);

    if (!ti_access_check(access, user, mask))
    {
        char * prefix, * name;
        size_t name_sz;
        access__helper_str(access, &prefix, &name, &name_sz);

        ex_set(e, EX_FORBIDDEN,
                "user `%.*s` is missing the required privileges (`%s`) "
                "on scope `%s%.*s`"DOC_GRANT,
                user->name->n, (char *) user->name->data,
                ti_auth_mask_to_str(mask),
                prefix,
                (int) name_sz, name);
    }
    return e->nr;
}

/* Returns 0 if "o" has less or equal access rights compared to "n".
 * The value is non-zero if "n" has rights which "o" does not have
 */
int ti_access_more_check(
        const vec_t * access,
        ti_user_t * o,
        ti_user_t * n,
        ex_t * e)
{
    uint16_t om = 0;
    uint16_t nm = 0;
    for (vec_each(access, ti_auth_t, auth))
    {
        if (auth->user == o)
            om = auth->mask;
        else if (auth->user == n)
            nm = auth->mask;
    }
    if (nm -= (om & nm))
    {
        char * prefix, * name;
        size_t name_sz;
        access__helper_str(access, &prefix, &name, &name_sz);

        ex_set(e, EX_FORBIDDEN,
                "user `%.*s` has privileges (`%s`) that user `%.*s` is "
                "missing on scope `%s%.*s`",
                (int) n->name->n, (char *) n->name->data,
                ti_auth_mask_to_str(nm),
                o->name->n, (char *) o->name->data,
                prefix,
                (int) name_sz, name);
    }
    return e->nr;
}

/*
 * Return 0 if the user has the required privileges or EX_FORBIDDEN if not
 * with e->msg set
 */
int ti_access_check_or_err(
        const vec_t * access,
        ti_user_t * user,
        uint64_t mask,
        ex_t * e)
{
    assert(e->nr == 0);

    if (!ti_access_check_or(access, user, mask))
    {
        char * prefix, * name;
        size_t name_sz;
        access__helper_str(access, &prefix, &name, &name_sz);

        ex_set(e, EX_FORBIDDEN,
                "user `%.*s` is missing the required privileges (`%s`) "
                "on scope `%s%.*s`"DOC_GRANT,
                (int) user->name->n, (char *) user->name->data,
                ti_auth_mask_to_str(mask),
                prefix,
                (int) name_sz, name);
    }
    return e->nr;
}



