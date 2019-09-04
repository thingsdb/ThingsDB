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
