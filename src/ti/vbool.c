/*
 * ti/vbool.c
 */
#include <tiinc.h>
#include <ti/val.h>
#include <ti/vbool.h>
#include <stdbool.h>

static ti_vbool_t vbool__true = {
    .ref = 1,
    .tp = TI_VAL_BOOL,
    .bool_ = true,
};

static ti_vbool_t vbool__false = {
    .ref = 1,
    .tp = TI_VAL_BOOL,
    .bool_ = false,
};

/*
 * This function is always successful and the result does not have to be
 * checked.
 */
ti_vbool_t * ti_vbool_get(_Bool b)
{
    ti_vbool_t * vbool = b ? &vbool__true : &vbool__false;
    ti_incref(vbool);
    return vbool;
}

/*
 * Can be used for a sanity check when stopping ThingsDB to see if all
 * references are cleared.
 */
_Bool ti_vbool_no_ref(void)
{
    return vbool__true.ref == 1 && vbool__false.ref == 1;
}
