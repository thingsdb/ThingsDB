/*
 * doc.inline.h
 */
#ifndef DOC_INLINE_H_
#define DOC_INLINE_H_

#include <doc.h>
#include <ti/member.t.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/varr.h>

/*
 * Some functions are available on different type. In this case we want to
 * return a link to the correct documentation. Since this requires ThingsDB
 * to check the type, the result of these documentation functions can be used
 * to find if the function is native available on the called type.
 */

static inline const char * doc_len(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NAME:
    case TI_VAL_STR:            return DOC_STR_LEN;
    case TI_VAL_BYTES:          return DOC_BYTES_LEN;
    case TI_VAL_MPDATA:         return DOC_MPDATA_LEN;
    case TI_VAL_THING:          return DOC_THING_LEN;
    case TI_VAL_ARR:            return DOC_LIST_LEN;
    case TI_VAL_SET:            return DOC_SET_LEN;
    case TI_VAL_MEMBER:         return doc_len(VMEMBER(val));
    default:                    return NULL;
    }
}

static inline const char * doc_filter(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return DOC_THING_FILTER;
    case TI_VAL_ARR:            return DOC_LIST_FILTER;
    case TI_VAL_SET:            return DOC_SET_FILTER;
    default:                    return NULL;
    }
}

static inline const char * doc_find(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ARR:            return DOC_LIST_FIND;
    case TI_VAL_SET:            return DOC_SET_FIND;
    default:                    return NULL;
    }
}

static inline const char * doc_some(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ARR:            return DOC_LIST_SOME;
    case TI_VAL_SET:            return DOC_SET_SOME;
    default:                    return NULL;
    }
}

static inline const char * doc_every(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ARR:            return DOC_LIST_EVERY;
    case TI_VAL_SET:            return DOC_SET_EVERY;
    default:                    return NULL;
    }
}

static inline const char * doc_reduce(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ARR:            return DOC_LIST_REDUCE;
    case TI_VAL_SET:            return DOC_SET_REDUCE;
    default:                    return NULL;
    }
}

static inline const char * doc_has(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return DOC_THING_HAS;
    case TI_VAL_SET:            return DOC_SET_HAS;
    default:                    return NULL;
    }
}

static inline const char * doc_map(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return DOC_THING_MAP;
    case TI_VAL_ARR:            return DOC_LIST_MAP;
    case TI_VAL_SET:            return DOC_SET_MAP;
    default:                    return NULL;
    }
}

static inline const char * doc_each(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return DOC_THING_EACH;
    case TI_VAL_ARR:            return DOC_LIST_EACH;
    case TI_VAL_SET:            return DOC_SET_EACH;
    default:                    return NULL;
    }
}

static inline const char * doc_copy(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return DOC_THING_COPY;
    case TI_VAL_WRAP:           return DOC_WTYPE_COPY;
    default:                    return NULL;
    }
}

static inline const char * doc_dup(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return DOC_THING_DUP;
    case TI_VAL_WRAP:           return DOC_WTYPE_DUP;
    default:                    return NULL;
    }
}


#endif  /* DOC_INLINE_H_ */
