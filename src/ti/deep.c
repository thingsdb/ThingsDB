#include <ti/deep.h>
#include <ti/val.inline.h>
#include <ti/vint.h>
#include <tiinc.h>
#include <doc.h>


int ti_deep_from_val(ti_val_t * val, uint8_t * deep, ex_t * e)
{
    int64_t deepi;

    if (!ti_val_is_int(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting `deep` to be of type `"TI_VAL_INT_S"` "
                "but got type `%s` instead",
                ti_val_str(val));
        return e->nr;
    }

    deepi = VINT(val);

    if (deepi < 0 || deepi > TI_MAX_DEEP_HINT)
    {
        ex_set(e, EX_VALUE_ERROR,
                "expecting a `deep` value between 0 and %d "
                "but got %"PRId64" instead",
                TI_MAX_DEEP_HINT, deepi);
        return e->nr;
    }

    *deep = (uint8_t) deepi;
    return 0;
}
