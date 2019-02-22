/*
 * ti/vint.c
 */
#include <tiinc.h>
#include <stdlib.h>
#include <ti/val.h>
#include <ti/vint.h>

static ti_vint_t vint__cache[32] = {
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -9 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -8 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -7 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -6 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -5 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -4 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -3 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -2 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -1 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 0  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 1  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 2  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 3  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 4  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 5  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 6  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 7  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 8  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 9  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 10  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 11  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 12  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 13  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 14  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 15  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 16  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 17  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 18  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 19  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 20  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 21  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 22  },
};


ti_vint_t * ti_vint_create(int64_t i)
{
    ti_vint_t * vint;
    if (i > 22 || i < -9)
    {
        vint = malloc(sizeof(ti_vint_t));
        if (!vint)
            return NULL;
        vint->ref = 1;
        vint->tp = TI_VAL_INT;
        vint->int_ = i;

        return vint;
    }

    vint = &vint__cache[i + 9];
    ti_incref(vint);
    return vint;
}

