/*
 * ti/types.inline.h
 */
#ifndef TI_TYPES_INLINE_H_
#define TI_TYPES_INLINE_H_

#include <util/smap.h>
#include <ti/type.h>
#include <ti/types.h>
#include <ti/raw.h>
#include <ti/val.inline.h>

static inline ti_raw_t * ti_type_nested_from_val(
    ti_type_t * type,
    ti_val_t * val,
    ex_t * e)
{
    return (ti_val_is_thing(val) || ti_val_is_array(val))
        ? ti__type_nested_from_val(type, val, e)
        : NULL;
}

static inline ti_type_t * ti_types_by_strn(ti_types_t * types, const char * str, size_t n)
{
    return smap_getn(types->smap, str, n);
}

static inline ti_type_t * ti_types_by_raw(ti_types_t * types, ti_raw_t * raw)
{
    return smap_getn(types->smap, (const char *) raw->data, raw->n);
}

static inline ti_type_t * ti_types_by_id(ti_types_t * types, uint16_t type_id)
{
    return imap_get(types->imap, type_id);
}

#endif /* TI_TYPES_INLINE_H_ */
