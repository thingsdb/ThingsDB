/*
 * ti/data.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/data.h>

qp_packer_t * ti_data_packer(size_t alloc_size, size_t init_nest_size)
{
    qp_packer_t * packer = qp_packer_create2(
            sizeof(ti_data_t) + alloc_size,
            init_nest_size);
    if (!packer)
        return NULL;
    packer->len = sizeof(ti_data_t);
    return packer;
}

ti_data_t * ti_data_from_packer(qp_packer_t * packer)
{
    ti_data_t * data = (ti_data_t *) packer->buffer;
    data->n = packer->len - sizeof(ti_data_t);
    packer->buffer = NULL;
    qp_packer_destroy(packer);
    return data;
}

ti_data_t * ti_data_for_set_job(ti_name_t * name, ti_val_t * val, int options)
{
    qp_packer_t * packer = ti_data_packer(512, 8);

    if (!packer)
        return NULL;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "set");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n))
        goto fail_packer;

    if (ti_val_to_packer(val, &packer, options))
        goto fail_packer;

    if (qp_close_map(packer) || qp_close_map(packer))
        goto fail_packer;

    return ti_data_from_packer(packer);

fail_packer:
    qp_packer_destroy(packer);
    return NULL;
}

ti_data_t * ti_data_for_del_job(const char * name, size_t n)
{
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(20 + n, 1);

    if (!packer)
        return NULL;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del");
    (void) qp_add_raw(packer, (const uchar *) name, n);
    (void) qp_close_map(packer);

    return ti_data_from_packer(packer);
}
