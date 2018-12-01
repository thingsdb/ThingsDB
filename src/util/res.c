/*
 * util/res.c
 */
#include <ti/name.h>
#include <ti/arrow.h>
#include <langdef/nd.h>
#include <util/omap.h>
#include <util/res.h>

int res_rval_clear(ti_res_t * res)
{
    if (res->rval)
    {
        ti_val_clear(res->rval);
        return 0;
    }
    res->rval = ti_val_create(TI_VAL_UNDEFINED, NULL);
    return res->rval ? 0 : -1;
}

void res_rval_destroy(ti_res_t * res)
{
    ti_val_destroy(res->rval);
    res->rval = NULL;
}

void res_rval_weak_destroy(ti_res_t * res)
{
    ti_val_weak_destroy(res->rval);
    res->rval = NULL;
}

