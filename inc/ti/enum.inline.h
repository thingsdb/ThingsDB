#include <ti/val.h>
#include <ti/enum.h>
#include <util/vec.h>


static inline ti_val_t * ti_enum_dval(ti_enum_t * enum_)
{
    ti_val_t * val = VEC_first(enum_->members);
    ti_incref(val);
    return val;
}
