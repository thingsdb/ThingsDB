/*
 * doc.inline.h
 */
#ifndef DOC_INLINE_H_
#define DOC_INLINE_H_

#include <doc.h>
#include <ti/val.h>
#include <ti/thing.h>
#include <ti/varr.h>

static inline const char * doc_len(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NAME:
    case TI_VAL_STR:            return DOC_STR_LEN;
    case TI_VAL_BYTES:          return DOC_BYTES_LEN;
    case TI_VAL_THING:          return ti_thing_is_object((ti_thing_t *) val)
                                    ? DOC_THING_LEN
                                    : DOC_TYPE_LEN;
    case TI_VAL_ARR:            return ti_varr_is_list((ti_varr_t *) val)
                                    ? DOC_LIST_LEN
                                    : DOC_TUPLE_LEN;
    case TI_VAL_SET:            return DOC_SET_LEN;
    case TI_VAL_ERROR:          return DOC_ERROR_LEN;
    default:                    return NULL;
    }
}

static inline const char * doc_filter(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return ti_thing_is_object((ti_thing_t *) val)
                                    ? DOC_THING_FILTER
                                    : DOC_TYPE_FILTER;
    case TI_VAL_ARR:            return ti_varr_is_list((ti_varr_t *) val)
                                    ? DOC_LIST_FILTER
                                    : DOC_TUPLE_FILTER;
    case TI_VAL_SET:            return DOC_SET_FILTER;
    default:                    return NULL;
    }
}

static inline const char * doc_find(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ARR:            return ti_varr_is_list((ti_varr_t *) val)
                                    ? DOC_LIST_FIND
                                    : DOC_TUPLE_FIND;
    case TI_VAL_SET:            return DOC_SET_FIND;
    default:                    return NULL;
    }
}

static inline const char * doc_findindex(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR
            ? ti_varr_is_list((ti_varr_t *) val)
            ? DOC_LIST_FINDINDEX
            : DOC_TUPLE_FINDINDEX
            : NULL;
}

static inline const char * doc_indexof(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR
            ? ti_varr_is_list((ti_varr_t *) val)
            ? DOC_LIST_INDEXOF
            : DOC_TUPLE_INDEXOF
            : NULL;
}

static inline const char * doc_sort(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR
            ? ti_varr_is_list((ti_varr_t *) val)
            ? DOC_LIST_SORT
            : DOC_TUPLE_SORT
            : NULL;
}


static inline const char * doc_get(ti_val_t * val)
{
    return val->tp == TI_VAL_THING
            ? ti_thing_is_object((ti_thing_t *) val)
            ? DOC_THING_GET
            : DOC_TYPE_GET
            : NULL;
}

static inline const char * doc_id(ti_val_t * val)
{
    return val->tp == TI_VAL_THING
            ? ti_thing_is_object((ti_thing_t *) val)
            ? DOC_THING_ID
            : DOC_TYPE_ID
            : NULL;
}

static inline const char * doc_keys(ti_val_t * val)
{
    return val->tp == TI_VAL_THING
            ? ti_thing_is_object((ti_thing_t *) val)
            ? DOC_THING_KEYS
            : DOC_TYPE_KEYS
            : NULL;
}

static inline const char * doc_values(ti_val_t * val)
{
    return val->tp == TI_VAL_THING
            ? ti_thing_is_object((ti_thing_t *) val)
            ? DOC_THING_VALUES
            : DOC_TYPE_VALUES
            : NULL;
}

static inline const char * doc_wrap(ti_val_t * val)
{
    return val->tp == TI_VAL_THING
            ? ti_thing_is_object((ti_thing_t *) val)
            ? DOC_THING_WRAP
            : DOC_TYPE_WRAP
            : NULL;
}

static inline const char * doc_has(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return ti_thing_is_object((ti_thing_t *) val)
                                    ? DOC_THING_HAS
                                    : DOC_TYPE_HAS;
    case TI_VAL_SET:            return DOC_SET_HAS;
    default:                    return NULL;
    }
}

static inline const char * doc_map(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:          return ti_thing_is_object((ti_thing_t *) val)
                                    ? DOC_THING_MAP
                                    : DOC_TYPE_MAP;
    case TI_VAL_ARR:            return ti_varr_is_list((ti_varr_t *) val)
                                    ? DOC_LIST_MAP
                                    : DOC_TUPLE_MAP;
    case TI_VAL_SET:            return DOC_SET_MAP;
    default:                    return NULL;
    }
}

#endif  /* DOC_INLINE_H_ */
