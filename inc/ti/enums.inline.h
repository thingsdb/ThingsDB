/*
 * ti/enums.inline.h
 */
#ifndef TI_ENUMS_INLINE_H_
#define TI_ENUMS_INLINE_H_

#include <util/smap.h>
#include <ti/enum.h>
#include <ti/enums.h>
#include <ti/raw.h>


static inline ti_enum_t * ti_enums_by_strn(ti_enums_t * enums, const char * str, size_t n)
{
    return smap_getn(enums->smap, str, n);
}

static inline ti_enum_t * ti_enums_by_raw(ti_enums_t * enums, ti_raw_t * raw)
{
    return smap_getn(enums->smap, (const char *) raw->data, raw->n);
}

static inline ti_enum_t * ti_enums_by_id(ti_enums_t * enums, uint16_t enum_id)
{
    return imap_get(enums->imap, enum_id);
}

#endif /* TI_ENUMS_INLINE_H_ */
