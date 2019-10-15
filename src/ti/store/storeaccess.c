/*
 * ti/store/access.c
 */
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/store/storeaccess.h>
#include <ti/users.h>
#include <util/fx.h>
#include <util/mpack.h>

int ti_store_access_store(const vec_t * access, const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "access") ||
        msgpack_pack_array(&pk, access->n)
    ) goto fail;

    for (vec_each(access, ti_auth_t, auth))
    {
        if (msgpack_pack_array(&pk, 2) ||
            msgpack_pack_uint64(&pk, auth->user->id) ||
            msgpack_pack_uint16(&pk, auth->mask)
        ) goto fail;
    }

    log_debug("stored access to file: `%s`", fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", fn);
done:
    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        return -1;
    }
    return 0;
}

int ti_store_access_restore(vec_t ** access, const char * fn)
{
    int rc = -1;
    size_t i;
    ssize_t n;
    mp_obj_t obj, mp_user_id, mp_mask;
    mp_unp_t up;
    ti_user_t * user;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    mp_unp_init(&up, data, (size_t) n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(&up, &mp_user_id) != MP_U64 ||
            mp_next(&up, &mp_mask) != MP_U64
        ) goto fail;

        user = ti_users_get_by_id(mp_user_id.via.u64);
        if (!user)
        {
            log_critical("missing user id: %"PRIu64, mp_user_id.via.u64);
            goto fail;
        }

        if (ti_access_grant(access, user, mp_mask.via.u64))
            goto fail;
    }

    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}
