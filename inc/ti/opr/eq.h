#include <ti/opr/oprinc.h>

static int opr__eq(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = ti_opr_eq(a, *b);

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;
}
