/*
 * ti/store/status.c
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
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (
        msgpack_pack_map(&pk, 2) ||
        mp_pack_str(&pk, "cevid") ||
        msgpack_pack_uint64(&pk, ti()->node->cevid) ||
        mp_pack_str(&pk, "next_thing_id") ||
        msgpack_pack_uint64(&pk, ti()->node->next_thing_id)
    ) goto fail;

    log_debug("stored status to file: `%s`", fn);
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

int ti_store_status_restore(const char * fn)
{
    int rc = -1;
    ssize_t n;
    mp_obj_t obj, mp_cevid, mp_next_thing_id;
    mp_unp_t up;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    mp_unp_init(&up, data, (size_t) n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &mp_cevid) != MP_U64 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &mp_next_thing_id) != MP_U64
    ) goto fail;

    ti()->node->sevid = ti()->node->cevid = mp_cevid.via.u64;
    ti()->events->next_event_id = mp_cevid.via.u64 + 1;
    ti()->node->next_thing_id = mp_next_thing_id.via.u64;

    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}
