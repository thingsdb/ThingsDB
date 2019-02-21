/*
 * ti/users.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/proto.h>
#include <ti/users.h>
#include <util/cryptx.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/vec.h>

static ti_users_t * users = NULL;

int ti_users_create(void)
{
    users = malloc(sizeof(ti_users_t));
    if (!users)
        goto failed;

    users->vec = vec_new(1);
    if (!users->vec)
        goto failed;

    ti()->users = users;
    return 0;

failed:
    free(users);
    users = NULL;
    return -1;
}

void ti_users_destroy(void)
{
    if (!users)
        return;
    vec_destroy(users->vec, (vec_destroy_cb) ti_user_drop);
    free(users);
    ti()->users = users = NULL;
}

ti_user_t * ti_users_new_user(
        const char * name,
        size_t n,
        const char * passstr,
        ex_t * e)
{
    ti_user_t * user = NULL;
    assert (e->nr == 0);

    if (!ti_user_name_check(name, n, e))
        goto done;

    if (!ti_user_pass_check(passstr, e))
        goto done;

    user = ti_user_create(ti_next_thing_id(), name, n, "");

    if (!user ||
        ti_user_set_pass(user, passstr) ||
        vec_push(&users->vec, user))
    {
        ti_user_drop(user);
        user = NULL;
        ex_set_alloc(e);
        goto done;
    }

done:
    return user;
}

ti_user_t * ti_users_load_user(
        uint64_t user_id,
        const char * name,
        size_t n,
        const char * encrypted,
        ex_t * e)
{
    ti_user_t * user = NULL;
    assert (e->nr == 0);

    if (!ti_user_name_check(name, n, e))
        goto done;

    if (ti_users_get_by_id(user_id))
    {
        ex_set(e, EX_INDEX_ERROR, "`user:%"PRIu64"` already exists", user_id);
        goto done;
    }

    if (user_id >= *ti()->next_thing_id)
        *ti()->next_thing_id = user_id + 1;

    user = ti_user_create(user_id, name, n, encrypted);

    if (!user || vec_push(&users->vec, user))
    {
        ti_user_drop(user);
        user = NULL;
        ex_set_alloc(e);
        goto done;
    }

done:
    return user;
}

void ti_users_del_user(ti_user_t * user)
{
    size_t i = 0;

    /* remove root access */
    ti_access_revoke(ti()->access, user, TI_AUTH_MASK_FULL);

    /* remove collection access */
    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        ti_access_revoke(collection->access, user, TI_AUTH_MASK_FULL);
    }

    /* remove user */
    for (vec_each(users->vec, ti_user_t, usr), ++i)
    {
        if (usr == user)
        {
            ti_user_drop(vec_remove(users->vec, i));
            return;
        }
    }
}

ti_user_t * ti_users_auth(qp_obj_t * name, qp_obj_t * pass, ex_t * e)
{
    char passbuf[ti_max_pass];
    char pw[CRYPTX_SZ];

    if (name->len < ti_min_name || name->len >= ti_max_name)
        goto failed;

    if (pass->len < ti_min_pass || pass->len >= ti_max_pass)
        goto failed;

    for (vec_each(users->vec, ti_user_t, user))
    {
        if (qpx_obj_eq_raw(name, user->name))
        {
            memcpy(passbuf, pass->via.raw, pass->len);
            passbuf[pass->len] = '\0';

            cryptx(passbuf, user->encpass, pw);
            if (strcmp(pw, user->encpass))
                goto failed;
            return user;
        }
    }

failed:
    ex_set(e, EX_AUTH_ERROR, "invalid username or password");
    return NULL;
}

/* Returns a borrowed reference */
ti_user_t * ti_users_get_by_id(uint64_t id)
{
    for (vec_each(users->vec, ti_user_t, user))
        if (user->id == id)
            return user;
    return NULL;
}

/* Returns a borrowed reference */
ti_user_t * ti_users_get_by_namestrn(const char * name, size_t n)
{
    for (vec_each(users->vec, ti_user_t, user))
        if (ti_raw_equal_strn(user->name, name, n))
            return user;
    return NULL;
}

ti_val_t * ti_users_as_qpval(void)
{
    ti_raw_t * rusers;
    qp_packer_t * packer = qp_packer_create2(4 + (192 * users->vec->n), 4);
    if (!packer)
        return NULL;

    (void) qp_add_array(&packer);

    for (vec_each(users->vec, ti_user_t, user))
        if (ti_user_to_packer(user, &packer))
            goto fail;

    if (qp_close_array(packer))
        goto fail;

    rusers = ti_raw_from_packer(packer);

fail:
    qp_packer_destroy(packer);
    return (ti_val_t *) rusers;
}


