/*
 * ti/store/storetimers.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/timer.h>
#include <ti/timers.h>
#include <ti/raw.inline.h>
#include <ti/store/storetimers.h>
#include <ti/val.inline.h>
#include <util/fx.h>
#include <util/mpack.h>

int ti_store_timers_store(vec_t * timers, const char * fn)
{
    ti_vp_t vp;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    uv_mutex_lock(ti.timers->lock);

    msgpack_packer_init(&vp.pk, f, msgpack_fbuffer_write);

    if (
        msgpack_pack_map(&vp.pk, 1) ||
        mp_pack_str(&vp.pk, "timers") ||
        msgpack_pack_array(&vp.pk, timers->n)
    ) goto fail;

    for (vec_each(timers, ti_timer_t, timer))
    {
        if (msgpack_pack_array(&vp.pk, 7) ||
            msgpack_pack_uint64(&vp.pk, timer->id) ||
            msgpack_pack_uint64(&vp.pk, timer->scope_id) ||
            msgpack_pack_uint64(&vp.pk, timer->next_run) ||
            msgpack_pack_uint32(&vp.pk, timer->repeat) ||
            msgpack_pack_uint64(&vp.pk, timer->user ? timer->user->id : 0) ||
            ti_closure_to_pk(timer->closure, &vp.pk, TI_VAL_PACK_FILE) ||
            msgpack_pack_array(&vp.pk, timer->args->n))
            goto fail;

        for (vec_each(timer->args, ti_val_t, val))
            if (ti_val_to_pk(val, &vp, TI_VAL_PACK_FILE))
                goto fail;
    }

    log_debug("stored timers to file: `%s`", fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", fn);
done:
    uv_mutex_unlock(ti.timers->lock);

    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        return -1;
    }

    (void) ti_sleep(5);
    return 0;
}

int ti_store_timers_restore(
        vec_t ** timers,
        const char * fn,
        ti_collection_t * collection)  /* collection may be NULL */
{
    int rc = -1;
    fx_mmap_t fmap;
    size_t i;
    mp_obj_t obj, mp_ver, mp_id, mp_scope_id, mp_next_run, mp_repeat,
             mp_user_id;
    mp_unp_t up;
    ti_closure_t * closure;
    ti_varr_t * varr;
    ti_timer_t * timer;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = &up,
    };

    /* lock the timers */
    uv_mutex_lock(ti.timers->lock);

    /* clear existing timers (may exist in the thingsdb scope) */
    ti_timers_clear(timers);

    if (!fx_file_exist(fn))
    {
        /*
         * TODO: (COMPAT) This check may be removed when we no longer require
         *       backwards compatibility with previous versions of ThingsDB
         *       where the modules file did not exist.
         */
        log_warning(
                "no timers found; "
                "file `%s` is missing",
                fn);

        uv_mutex_unlock(ti.timers->lock);
        return ti_store_timers_store(*timers, fn);
    }

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail1;

    if (vec_reserve(timers, obj.via.sz))
        goto fail1;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 7 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_scope_id) != MP_U64 ||
            mp_next(&up, &mp_next_run) != MP_U64 ||
            mp_next(&up, &mp_repeat) != MP_U64 ||
            mp_next(&up, &mp_user_id) != MP_U64
        ) goto fail1;

        closure = (ti_closure_t *) ti_val_from_vup(&vup);
        varr = (ti_varr_t *) ti_val_from_vup(&vup);

        if (!closure || !ti_val_is_closure((ti_val_t *) closure) ||
            !varr || !ti_val_is_array((ti_val_t *) varr))
            goto fail2;

        timer = ti_timer_create(
                    mp_id.via.u64,
                    mp_next_run.via.u64,
                    mp_repeat.via.u64,
                    mp_scope_id.via.u64,
                    ti_users_get_by_id(mp_user_id.via.u64),
                    closure,
                    varr->vec);
        if (!timer)
            goto fail2;

        ti_decref(closure);
        free(varr);
        VEC_push(*timers, timer);
    }

    rc = 0;
    goto done;

fail2:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) varr);

done:
fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    uv_mutex_unlock(ti.timers->lock);
    return rc;
}
