/*
 * ti/varr.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/varr.h>
#include <ti/val.h>
#include <util/logger.h>


ti_varr_t * ti_varr_create(size_t sz)
{
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARRAY;

    varr->vec = vec_new(sz);
    if (!varr->vec)
    {
        free(varr);
        return NULL;
    }

    return varr;
}


void ti_varr_destroy(ti_varr_t * varr)
{
    vec_destroy(varr->vec, (vec_destroy_cb) ti_val_drop);
}
