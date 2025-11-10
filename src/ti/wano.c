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

int ti_wano_copy(ti_wano_t ** wano, uint8_t deep)
{
    ti_thing_t * thing = (*wano)->thing;
    ti_wano_t * _wano = ti_wano_create(thing, (*wano)->ano);
    if (!_wano)
        return -1;

    if (ti_thing_copy(&_wano->thing, deep))
    {
        ti_val_unsafe_drop((ti_val_t *) _wano);
        return -1;
    }
    ti_val_unsafe_drop((ti_val_t *) *wano);
    *wano = _wano;
    return 0;
}

int ti_wano_dup(ti_wano_t ** wano, uint8_t deep)
{
    assert(deep);
    ti_wano_t * _wano = ti_wano_create((*wano)->thing, (*wano)->ano);
    if (!_wano)
        return -1;

    if (ti_thing_dup(&_wano->thing, deep))
    {
        ti_wano_destroy(_wano);
        return -1;
    }

    ti_val_unsafe_drop((ti_val_t *) *wano);
    *wano = _wano;
    return 0;
}