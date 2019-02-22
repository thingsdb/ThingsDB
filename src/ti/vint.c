/*
 * ti/vint.c
 */
#include <tiinc.h>
#include <stdlib.h>
#include <ti/val.h>
#include <ti/vint.h>

static ti_vint_t vint__n1 = {
        .ref = 1,
        .tp = TI_VAL_INT,
        .int_ = -1,
};

static ti_vint_t vint__0 = {
        .ref = 1,
        .tp = TI_VAL_INT,
        .int_ = 0,
};

static ti_vint_t vint__1 = {
        .ref = 1,
        .tp = TI_VAL_INT,
        .int_ = 1,
};

ti_vint_t * ti_vint_create(int64_t i)
{
    ti_vint_t * vint;
    switch (i)
    {
    case -1:
        vint = &vint__n1;
        break;
    case 0:
        vint = &vint__0;
        break;
    case 1:
        vint = &vint__1;
        break;
    default:
        vint = malloc(sizeof(ti_vint_t));
        if (!vint)
            return NULL;
        vint->ref = 1;
        vint->tp = TI_VAL_INT;
        vint->int_ = i;
        return vint;
    }

    ti_incref(vint);
    return vint;
}

