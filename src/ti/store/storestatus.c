/*
 * ti/store/storestatus.c
 */
#include <ti.h>
#include <ti/store/storestatus.h>
#include <util/fx.h>
#include <util/mpack.h>

int ti_store_status_store(const char * fn)
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
        msgpack_pack_map(&pk, 2) ||
        mp_pack_str(&pk, "ccid") ||
        msgpack_pack_uint64(&pk, ti.node->ccid) ||
        mp_pack_str(&pk, "next_free_id") ||
        msgpack_pack_uint64(&pk, ti.node->next_free_id)
    ) goto fail;

    log_debug("stored status to file: `%s`", fn);
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

int ti_store_status_restore(const char * fn)
{
    int rc = -1;
    ssize_t n;
    mp_obj_t obj, mp_ccid, mp_next_thing_id;
    mp_unp_t up;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    mp_unp_init(&up, data, (size_t) n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &mp_ccid) != MP_U64 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &mp_next_thing_id) != MP_U64
    ) goto fail;

    ti.node->scid = ti.node->ccid = mp_ccid.via.u64;
    ti.changes->next_change_id = mp_ccid.via.u64 + 1;
    ti.node->next_free_id = mp_next_thing_id.via.u64;

    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}
