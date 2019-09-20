/*
 * ti/types.h
 */
#ifndef TI_TYPES_H_
#define TI_TYPES_H_

typedef struct ti_types_s ti_types_t;

#include <stdint.h>
#include <util/smap.h>
#include <util/imap.h>
#include <ti/type.h>
#include <ex.h>

ti_types_t * ti_types_create(void);
void ti_types_destroy(ti_types_t * types);
uint16_t ti_types_get_new_id(ti_types_t * types, ex_t * e);

static inline ti_type_t * ti_types_by_raw(ti_types_t * types, ti_raw_t * raw);

struct ti_types_s
{
    imap_t * imap;
    smap_t * smap;
};

static inline ti_type_t * ti_types_by_raw(ti_types_t * types, ti_raw_t * raw)
{
    return smap_getn(types->smap, (const char *) raw->data, raw->n);
}

#endif /* TI_TYPES_H_ */
