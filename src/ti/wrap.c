/*
 * ti/wrap.c
 */
#include <ti/wrap.h>


ti_wrap_t * ti_wrap_create(ti_thing_t * thing, uint16_t type_id)
{
    ti_wrap_t * wrap = malloc(sizeof(ti_wrap_t));
    if (!wrap)
        return NULL;

    wrap->ref = 1;
    wrap->tp = TI_VAL_WRAP;
    wrap->type_id = type_id;
    wrap->thing = thing;

    ti_incref(thing);

    return wrap;
}

void ti_wrap_destroy(ti_wrap_t * wrap)
{
    if (!wrap)
        return;
    ti_val_drop((ti_val_t *) wrap->thing);
    free(wrap);
}

int ti_wrap_to_packer(ti_wrap_t * wrap, qp_packer_t ** pckr, int options)
{
    ti_type_t * type = ti_wrap_type(wrap);

    return 0;
}
