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
