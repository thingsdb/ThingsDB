/*
 * ti/vbool.c
 */
#include <tiinc.h>
#include <ti/val.h>
#include <ti/vbool.h>
#include <stdbool.h>

ti_vbool_t vbool__true = {
    .ref = 1,
    .tp = TI_VAL_BOOL,
    .bool_ = true,
};

ti_vbool_t vbool__false = {
    .ref = 1,
    .tp = TI_VAL_BOOL,
    .bool_ = false,
};

/*
 * Can be used for a sanity check when stopping ThingsDB to see if all
 * references are cleared.
 */
_Bool ti_vbool_no_ref(void)
{
    return vbool__true.ref == 1 && vbool__false.ref == 1;
}
