/*
 * ti/store/storemodules.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/store/storeusers.h>
#include <ti/users.h>
#include <ti/token.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <util/vec.h>

static int store_modules__store_cb(ti_module_t * module, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 5) ||
        mp_pack_strn(pk, module->name->str, module->name->n) ||
        mp_pack_str(pk, module->fn) ||
        msgpack_pack_uint64(pk, module->created_at) ||
        module->conf_pkg
            ? mp_pack_bin(
                    pk,
                    module->conf_pkg,
                    sizeof(ti_pkg_t) + module->conf_pkg->n)
            : msgpack_pack_nil(pk) ||
        module->scope_id
            ? msgpack_pack_uint64(pk, *module->scope_id)
            : msgpack_pack_nil(pk)
    );
}

int ti_store_modules_store(const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "modules") ||
        msgpack_pack_array(&pk, ti.modules->n)
    ) goto fail;

    if (smap_values(ti.modules, (smap_val_cb) store_modules__store_cb, &pk))
        goto fail;

    log_debug("stored modules to file: `%s`", fn);
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

int ti_store_modules_restore(const char * fn)
{
    int rc = -1;
    size_t i;
    ssize_t n;
    mp_obj_t obj, mp_name, mp_file, mp_created, mp_pkg, mp_scope;
    mp_unp_t up;
    ti_module_t * module;
    ti_pkg_t * conf_pkg;
    uint64_t * scope_id;
    uchar * data;

    if (!fx_file_exist(fn))
    {
        /*
         * TODO: (COMPAT) This check may be removed when we no longer require
         *       backwards compatibility with previous versions of ThingsDB
         *       where the modules file did not exist.
         */
        log_warning(
                "no modules found; "
                "file `%s` is missing",
                fn);
        return ti_store_modules_store(fn);
    }


    data = fx_read(fn, &n);
    if (!data)
        return -1;

    mp_unp_init(&up, data, (size_t) n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_next(&up, &mp_file) != MP_STR ||
            mp_next(&up, &mp_created) != MP_U64 ||
            mp_next(&up, &mp_pkg) > MP_END ||
            mp_next(&up, &mp_scope) > MP_END
        ) goto fail;

        if (mp_pkg.tp == MP_BIN)
        {
            conf_pkg = malloc(mp_pkg.via.bin.n);
            if (!conf_pkg)
                goto fail;
            memcpy(conf_pkg, mp_pkg.via.bin.data, mp_pkg.via.bin.n);
        }
        else
            conf_pkg = NULL;

        if (mp_scope.tp == MP_U64)
        {
            scope_id = malloc(sizeof(uint64_t));
            if (!scope_id)
            {
                free(conf_pkg);
                goto fail;
            }
            *scope_id = mp_scope.via.u64;
        }
        else
            scope_id = NULL;

        module = ti_module_create(
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_file.via.str.data,
                mp_file.via.str.n,
                mp_created.via.u64,
                conf_pkg,
                scope_id);

        if (!module)
        {
            free(conf_pkg);
            free(scope_id);
            goto fail;
        }
    }

    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}
