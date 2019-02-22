/*
 * ti/vfloat.c
 */
#include <tiinc.h>
#include <stdlib.h>
#include <ti/val.h>
#include <ti/vfloat.h>


static ti_vfloat_t vfloat__0 = {
        .ref = 1,
        .tp = TI_VAL_FLOAT,
        .float_ = 0.0f,
};


ti_vfloat_t * ti_vfloat_create(double d)
{
    ti_vfloat_t * vfloat;
    if (!d)
    {
        vfloat = &vfloat__0;
        ti_incref(vfloat);
        return vfloat;
    }

    vfloat = malloc(sizeof(ti_vfloat_t));
    if (!vfloat)
        return NULL;

    vfloat->ref = 1;
    vfloat->tp = TI_VAL_FLOAT;
    vfloat->float_ = d;

    return vfloat;
}

