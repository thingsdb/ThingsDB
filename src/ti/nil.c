/*
 * ti/nil.c
 */
#include <ti/varr.h>
#include <ti/val.h>

static ti_nil_t nil__val = {
        .ref = 1,
        .tp = TI_VAL_NIL,
        .nil = NULL,
};

ti_nil_t * ti_nil_get(void)
{
    ti_incref(&nil__val);
    return &nil__val;
}
