/*
 * ti/val.inline.h
 */
#ifndef TI_VAL_INLINE_H_
#define TI_VAL_INLINE_H_

#include <ti/thing.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <ti/closure.h>
#include <ex.h>

static inline _Bool ti_val_is_object(ti_val_t * val)
{
    return val->tp == TI_VAL_THING && ti_thing_is_object((ti_thing_t *) val);
}

static inline _Bool ti_val_is_instance(ti_val_t * val)
{
    return val->tp == TI_VAL_THING && ti_thing_is_instance((ti_thing_t *) val);
}

/*
 * although the underlying pointer might point to a new value after calling
 * this function, the `old` pointer value can still be used and has at least
 * one reference left.
 *
 * errors:
 *      - memory allocation (vector / set /closure creation)
 *      - lock/syntax/bad data errors in closure
 */
static inline int ti_val_make_assignable(ti_val_t ** val, ex_t * e)
{
    switch ((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ERROR:
        return 0;
    case TI_VAL_ARR:
        if (ti_varr_to_list((ti_varr_t **) val))
            ex_set_mem(e);
        return e->nr;
    case TI_VAL_SET:
        if (ti_vset_assign((ti_vset_t **) val))
            ex_set_mem(e);
        return e->nr;
    case TI_VAL_CLOSURE:
        return ti_closure_unbound((ti_closure_t * ) *val, e);
    }
    assert(0);
    return -1;
}


#endif  /* TI_VAL_INLINE_H_ */
