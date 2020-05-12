/*
 * ti/venum.c
 */

#include <ti/venum.h>

/*
 * Increases the reference of value by one on success.
 */
ti_venum_t * ti_venum_create(ti_enum_t * enum_, ti_val_t * val, uint16_t idx)
{
    ti_venum_t * venum = malloc(sizeof(ti_venum_t));
    if (!venum)
        return NULL;

    venum->ref = 1;
    venum->tp = TI_VAL_ENUM;

    venum->val = val;
    venum->enum_ = enum_;
}
