/*
 * ti/wano.c
 */
#include <ti/wano.h>
#include <ti/val.inline.h>


ti_wano_t * ti_wano_create(ti_thing_t * thing, ti_ano_t * ano)
{
    ti_wano_t * wano = malloc(sizeof(ti_wano_t));
    if (!wano)
        return NULL;
    wano->ref = 1;
    wano->tp = TI_VAL_WANO;
    wano->ano = ano;
    wano->thing = thing;
    ti_incref(ano);
    ti_incref(thing);
    return wano;
}

void ti_wano_destroy(ti_wano_t * wano)
{
    ti_val_unsafe_drop((ti_val_t *) wano->ano);
    ti_val_unsafe_gc_drop((ti_val_t *) wano->thing);
    free(wano);
}
