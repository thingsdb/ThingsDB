#include <ti/flags.h>
#include <ti/val.inline.h>
#include <ti/vint.h>
#include <tiinc.h>
#include <doc.h>


int ti_flags_set_from_val(ti_val_t * val, uint8_t * flags, ex_t * e)
{
    if (!ti_val_is_int(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting `flags` to be of type `"TI_VAL_INT_S"` "
                "but got type `%s` instead",
                ti_val_str(val));
        return e->nr;
    }

    *flags |= VINT(val) & TI_FLAGS_NO_IDS;
    return 0;
}
