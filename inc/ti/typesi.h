/*
 * ti/typesi.h
 */
#ifndef TI_TYPESI_H_
#define TI_TYPESI_H_

#include <util/smap.h>
#include <ti/type.h>
#include <ti/types.h>
#include <ti/raw.h>


static inline ti_type_t * ti_types_by_raw(ti_types_t * types, ti_raw_t * raw)
{
    return smap_getn(types->smap, (const char *) raw->data, raw->n);
}

#endif /* TI_TYPESI_H_ */
