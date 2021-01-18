/*
 * ti/store/storenames.c
 */
#include <ti.h>
#include <ti/names.h>
#include <ti/store/storenames.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/smap.h>
#include <util/mpack.h>

static inline int names__write_cb(ti_name_t * name, msgpack_packer * pk);

int ti_store_names_store(const char * fn)
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
        msgpack_pack_array(&pk, ti.names->n) ||
        smap_values(ti.names, (smap_val_cb) names__write_cb, &pk)
    ) goto fail;

    log_debug("stored names to file: `%s`", fn);
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

imap_t * ti_store_names_restore(const char * fn)
{
    size_t i;
    ssize_t n;
    mp_obj_t obj, mp_uintptr, mp_name;
    mp_unp_t up;
    ti_name_t * name;
    imap_t * namesmap = imap_create();
    uchar * data = fx_read(fn, &n);
    if (!data || !namesmap)
        goto fail;

    mp_unp_init(&up, data, (size_t) n);

    if (mp_next(&up, &obj) != MP_ARR)
        goto fail;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(&up, &mp_uintptr) != MP_U64 ||
            mp_next(&up, &mp_name) != MP_STR
        ) goto fail;

        name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
        if (!name || imap_add(namesmap, mp_uintptr.via.u64, name))
            goto fail;
    }

    goto done;

fail:
    log_critical("failed to restore names from file: `%s`", fn);
    imap_destroy(namesmap, NULL);
    namesmap = NULL;
done:
    free(data);
    return namesmap;
}

static inline int names__write_cb(ti_name_t * name, msgpack_packer * pk)
{
    uintptr_t p = (uintptr_t) name;
    return name->str[0] == '$' ? 0 : (
        msgpack_pack_array(pk, 2) ||
        msgpack_pack_uint64(pk, p) ||
        mp_pack_strn(pk, name->str, name->n)
    );
}
