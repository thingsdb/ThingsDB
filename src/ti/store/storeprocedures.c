/*
 * ti/store/procedures.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/raw.inline.h>
#include <ti/store/storeprocedures.h>
#include <util/fx.h>
#include <util/mpack.h>

int ti_store_procedures_store(const vec_t * procedures, const char * fn)
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
        mp_pack_str(&pk, "procedures") ||
        msgpack_pack_array(&pk, procedures->n)
    ) goto fail;

    for (vec_each(procedures, ti_procedure_t, procedure))
    {
        if (
            msgpack_pack_array(&pk, 3) ||
            mp_pack_strn(&pk, procedure->name->data, procedure->name->n) ||
            msgpack_pack_uint64(&pk, procedure->created_at) ||
            ti_closure_to_pk(procedure->closure, &pk)
        ) goto fail;
    }

    log_debug("stored procedures to file: `%s`", fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", fn);
done:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        return -1;
    }
    return 0;
}

int ti_store_procedures_restore(
        vec_t ** procedures,
        const char * fn,
        ti_collection_t * collection)  /* collection may be NULL */
{
    int rc = -1;
    fx_mmap_t fmap;
    size_t i;
    mp_obj_t obj, mp_ver, mp_name, mp_created;
    mp_unp_t up;
    ti_raw_t * rname;
    ti_closure_t * closure;
    ti_procedure_t * procedure;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = &up,
    };

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail1;

    for (i = obj.via.sz; i--;)
    {
        if (
            mp_next(&up, &obj) != MP_ARR || obj.via.sz != 3 ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_next(&up, &mp_created) != MP_U64
        ) goto fail1;

        rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
        closure = (ti_closure_t *) ti_val_from_unp(&vup);
        procedure = NULL;

        if (!rname ||
            !closure ||
            !ti_val_is_closure((ti_val_t *) closure) ||
            !(procedure = ti_procedure_create(
                    rname,
                    closure,
                    mp_created.via.u64)) ||
            ti_procedures_add(procedures, procedure))
            goto fail2;

        ti_decref(rname);
        ti_decref(closure);
    }

    rc = 0;
    goto done;

fail2:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) rname);
    ti_val_drop((ti_val_t *) closure);

done:
fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    return rc;
}
