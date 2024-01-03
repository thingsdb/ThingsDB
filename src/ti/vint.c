/*
 * ti/vint.c
 */
#include <tiinc.h>
#include <stdlib.h>
#include <ti/val.h>
#include <ti/vint.h>

#define VX(x) \
    {.ref = 1, .tp = TI_VAL_INT, .int_ = x }

/* PRE-allocated integer values */
static ti_vint_t vint__cache[256] = {

        VX(0x00), VX(0x01), VX(0x02), VX(0x03),
        VX(0x04), VX(0x05), VX(0x06), VX(0x07),
        VX(0x08), VX(0x09), VX(0x0a), VX(0x0b),
        VX(0x0c), VX(0x0d), VX(0x0e), VX(0x0f),

        VX(0x10), VX(0x11), VX(0x12), VX(0x13),
        VX(0x14), VX(0x15), VX(0x16), VX(0x17),
        VX(0x18), VX(0x19), VX(0x1a), VX(0x1b),
        VX(0x1c), VX(0x1d), VX(0x1e), VX(0x1f),

        VX(0x20), VX(0x21), VX(0x22), VX(0x23),
        VX(0x24), VX(0x25), VX(0x26), VX(0x27),
        VX(0x28), VX(0x29), VX(0x2a), VX(0x2b),
        VX(0x2c), VX(0x2d), VX(0x2e), VX(0x2f),

        VX(0x30), VX(0x31), VX(0x32), VX(0x33),
        VX(0x34), VX(0x35), VX(0x36), VX(0x37),
        VX(0x38), VX(0x39), VX(0x3a), VX(0x3b),
        VX(0x3c), VX(0x3d), VX(0x3e), VX(0x3f),

        VX(0x40), VX(0x41), VX(0x42), VX(0x43),
        VX(0x44), VX(0x45), VX(0x46), VX(0x47),
        VX(0x48), VX(0x49), VX(0x4a), VX(0x4b),
        VX(0x4c), VX(0x4d), VX(0x4e), VX(0x4f),

        VX(0x50), VX(0x51), VX(0x52), VX(0x53),
        VX(0x54), VX(0x55), VX(0x56), VX(0x57),
        VX(0x58), VX(0x59), VX(0x5a), VX(0x5b),
        VX(0x5c), VX(0x5d), VX(0x5e), VX(0x5f),

        VX(0x60), VX(0x61), VX(0x62), VX(0x63),
        VX(0x64), VX(0x65), VX(0x66), VX(0x67),
        VX(0x68), VX(0x69), VX(0x6a), VX(0x6b),
        VX(0x6c), VX(0x6d), VX(0x6e), VX(0x6f),

        VX(0x70), VX(0x71), VX(0x72), VX(0x73),
        VX(0x74), VX(0x75), VX(0x76), VX(0x77),
        VX(0x78), VX(0x79), VX(0x7a), VX(0x7b),
        VX(0x7c), VX(0x7d), VX(0x7e), VX(0x7f),

        VX(-0x80), VX(-0x7f), VX(-0x7e), VX(-0x7d),
        VX(-0x7c), VX(-0x7b), VX(-0x7a), VX(-0x79),
        VX(-0x78), VX(-0x77), VX(-0x76), VX(-0x75),
        VX(-0x74), VX(-0x73), VX(-0x72), VX(-0x71),

        VX(-0x70), VX(-0x6f), VX(-0x6e), VX(-0x6d),
        VX(-0x6c), VX(-0x6b), VX(-0x6a), VX(-0x69),
        VX(-0x68), VX(-0x67), VX(-0x66), VX(-0x65),
        VX(-0x64), VX(-0x63), VX(-0x62), VX(-0x61),

        VX(-0x60), VX(-0x5f), VX(-0x5e), VX(-0x5d),
        VX(-0x5c), VX(-0x5b), VX(-0x5a), VX(-0x59),
        VX(-0x58), VX(-0x57), VX(-0x56), VX(-0x55),
        VX(-0x54), VX(-0x53), VX(-0x52), VX(-0x51),

        VX(-0x50), VX(-0x4f), VX(-0x4e), VX(-0x4d),
        VX(-0x4c), VX(-0x4b), VX(-0x4a), VX(-0x49),
        VX(-0x48), VX(-0x47), VX(-0x46), VX(-0x45),
        VX(-0x44), VX(-0x43), VX(-0x42), VX(-0x41),

        VX(-0x40), VX(-0x3f), VX(-0x3e), VX(-0x3d),
        VX(-0x3c), VX(-0x3b), VX(-0x3a), VX(-0x39),
        VX(-0x38), VX(-0x37), VX(-0x36), VX(-0x35),
        VX(-0x34), VX(-0x33), VX(-0x32), VX(-0x31),

        VX(-0x30), VX(-0x2f), VX(-0x2e), VX(-0x2d),
        VX(-0x2c), VX(-0x2b), VX(-0x2a), VX(-0x29),
        VX(-0x28), VX(-0x27), VX(-0x26), VX(-0x25),
        VX(-0x24), VX(-0x23), VX(-0x22), VX(-0x21),

        VX(-0x20), VX(-0x1f), VX(-0x1e), VX(-0x1d),
        VX(-0x1c), VX(-0x1b), VX(-0x1a), VX(-0x19),
        VX(-0x18), VX(-0x17), VX(-0x16), VX(-0x15),
        VX(-0x14), VX(-0x13), VX(-0x12), VX(-0x11),

        VX(-0x10), VX(-0x0f), VX(-0x0e), VX(-0x0d),
        VX(-0x0c), VX(-0x0b), VX(-0x0a), VX(-0x09),
        VX(-0x08), VX(-0x07), VX(-0x06), VX(-0x05),
        VX(-0x04), VX(-0x03), VX(-0x02), VX(-0x01),
};

/*
 * Values of i in the range -128-127 do not have to be checked. Other input
 * will be allocated and thus needs to be checked for NULL.
 */
ti_vint_t * ti_vint_create(int64_t i)
{
    ti_vint_t * vint;
    if (-(1<<7) <= i && i < 1<<7)
    {
        /* Integer values in the range -128-127 are allocated on the stack */
        uint8_t x = (uint8_t) i;
        vint = &vint__cache[x];
        ti_incref(vint);
        return vint;
    }

    vint = malloc(sizeof(ti_vint_t));
    if (!vint)
        return NULL;
    vint->ref = 1;
    vint->tp = TI_VAL_INT;
    vint->int_ = i;

    return vint;
}

ti_vint_t * ti_vint_borrow_zero(void)
{
    return &vint__cache[0];
}

_Bool ti_vint_no_ref(void)
{
    for (size_t i = 0; i < 256; ++i)
        if (vint__cache[i].ref != 1)
            return false;
    return true;
}

