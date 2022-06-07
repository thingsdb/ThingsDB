/*
 * ti/store/storevtasks.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/raw.inline.h>
#include <ti/store/storetasks.h>
#include <ti/tasks.h>
#include <ti/val.inline.h>
#include <ti/vtask.h>
#include <ti/vtask.inline.h>
#include <util/fx.h>
#include <util/mpack.h>

int ti_store_tasks_store(vec_t * vtasks, const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (
        msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "tasks") ||
        msgpack_pack_array(&pk, vtasks->n)
    ) goto fail;

    for (vec_each(vtasks, ti_vtask_t, vtask))
    {
        assert (vtask->id);  /* only tasks with an Id exist in the vector */
        if (msgpack_pack_array(&pk, 6) ||
            msgpack_pack_uint64(&pk, vtask->id) ||
            msgpack_pack_uint64(&pk, vtask->run_at) ||
            msgpack_pack_uint64(&pk, vtask->user->id) ||
            ti_closure_to_store_pk(vtask->closure, &pk) ||
            (vtask->verr
                ? ti_verror_to_store_pk(vtask->verr, &pk)
                : msgpack_pack_nil(&pk)) ||
            msgpack_pack_array(&pk, vtask->args->n))
            goto fail;

        for (vec_each(vtask->args, ti_val_t, val))
            if (ti_val_to_store_pk(val, &pk))
                goto fail;
    }

    log_debug("stored tasks to file: `%s`", fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", fn);
done:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        return -1;
    }

    (void) ti_sleep(5);
    return 0;
}

int ti_store_tasks_restore(
        vec_t ** vtasks,
        const char * fn,
        ti_collection_t * collection)  /* collection may be NULL */
{
    int rc = -1;
    const char * keep;
    fx_mmap_t fmap;
    size_t i, j;
    mp_obj_t obj, mp_id, mp_run_at, mp_user_id;
    mp_unp_t up;
    ti_closure_t * closure;
    ti_varr_t * varr = NULL;
    ti_vtask_t * vtask;
    ti_verror_t * verr;
    ti_user_t * user;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = &up,
    };


    /* clear existing vtasks (may exist in the thingsdb scope) */
    ti_tasks_clear_dropped(vtasks);

    if (!fx_file_exist(fn))
    {
        /*
         * TODO: (COMPAT) This check may be removed when we no longer require
         *       backwards compatibility with previous versions of ThingsDB
         *       where the tasks file did not exist.
         */
        log_warning("no tasks found; file `%s` is missing", fn);
        return ti_store_tasks_store(*vtasks, fn);
    }

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail1;

    if (vec_reserve(vtasks, obj.via.sz))
        goto fail1;

    keep = up.pt;

    for (j = i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 6 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_run_at) != MP_U64 ||
            mp_next(&up, &mp_user_id) != MP_U64)
            goto fail1;

        user = ti_users_get_by_id(mp_user_id.via.u64);
        closure = (ti_closure_t *) ti_val_from_vup(&vup);
        verr = mp_skip_nil(&up)
                ? NULL
                : (ti_verror_t *) ti_val_from_vup(&vup);

        mp_skip(&up);  /* varr */

        if (!user ||
            (!closure || !ti_val_is_closure((ti_val_t *) closure)) ||
            (verr && !ti_val_is_error((ti_val_t *) verr)))
            goto fail2;

        vtask = ti_vtask_create(
                    mp_id.via.u64,
                    mp_run_at.via.u64,
                    user,
                    closure,
                    verr,
                    NULL);
        if (!vtask)
            goto fail2;

        ti_decref(closure);
        if (verr)
            ti_decref(verr);

        /* push the task to the list, use ti_tasks_append() to re-schedule
         * at startup, bug #248 */
        (void) ti_tasks_append(vtasks, vtask);
    }

    up.pt = keep;

    for (; j--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 6 ||
            mp_next(&up, &mp_id) != MP_U64)
            goto fail1;

        mp_skip(&up);  /* run_at */
        mp_skip(&up);  /* user_id */
        mp_skip(&up);  /* closure */
        mp_skip(&up);  /* verr */

        varr = (ti_varr_t *) ti_val_from_vup(&vup);

        if (!varr || !ti_val_is_array((ti_val_t *) varr))
            goto fail2;

        vtask = ti_vtask_by_id(*vtasks, mp_id.via.u64);
        if (!vtask)
            goto fail2;

        vtask->args = varr->vec;
        free(varr);
    }

    rc = 0;
    goto done;

fail2:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) verr);
    ti_val_drop((ti_val_t *) varr);

done:
fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    return rc;
}
