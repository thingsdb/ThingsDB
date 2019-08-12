/*
 * ti/vint.c
 */
#include <tiinc.h>
#include <stdlib.h>
#include <ti/val.h>
#include <ti/vint.h>

/* PRE-allocated integer values */
static ti_vint_t vint__cache[64] = {
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -11 },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = -10 },
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
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 23  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 24  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 25  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 26  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 27  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 28  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 29  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 30  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 31  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 32  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 33  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 34  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 35  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 36  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 37  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 38  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 39  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 40  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 41  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 42  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 43  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 44  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 45  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 46  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 47  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 48  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 49  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 50  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 51  },
        {.ref = 1, .tp = TI_VAL_INT, .int_ = 52  },
};


ti_vint_t * ti_vint_create(int64_t i)
{
    ti_vint_t * vint;
    if (i > 52 || i < -11)
    {
        vint = malloc(sizeof(ti_vint_t));
        if (!vint)
            return NULL;
        vint->ref = 1;
        vint->tp = TI_VAL_INT;
        vint->_flags = 0;
        vint->int_ = i;

        return vint;
    }

    vint = &vint__cache[i + 11];
    ti_incref(vint);
    return vint;
}

