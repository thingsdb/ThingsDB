/*
 * ti/data.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/data.h>
#include <util/mpack.h>

void ti_data_init(ti_data_t * data, size_t total_n)
{
    data->n = total_n - sizeof(ti_data_t);
}

ti_data_t * ti_data_for_set_job(ti_name_t * name, ti_val_t * val, int options)
{
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_data_t), sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "set") ||
        msgpack_pack_map(&pk, 1)
    ) goto fail_pack;

    if (mp_pack_strn(&pk, name->str, name->n) ||
        ti_val_to_pk(val, &pk, options)
    ) goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    return data;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return NULL;
}

ti_data_t * ti_data_for_del_job(const char * name, size_t n)
{
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, 64 + n, sizeof(ti_data_t)))
        return NULL;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "del");
    mp_pack_strn(&pk, name, n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    return data;
}
