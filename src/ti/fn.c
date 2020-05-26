#include <ti/fn/fn.h>

int fn_arg_str_slow(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str(val))

        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}
