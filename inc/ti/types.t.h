/*
 * ti/types.t.h
 */
#ifndef TI_TYPES_T_H_
#define TI_TYPES_T_H_

#define TI_TYPES_RM_FLAG 0x8000
#define TI_TYPES_RM_MASK 0x7fff

typedef struct ti_types_s ti_types_t;

#include <inttypes.h>
#include <ti/collection.t.h>
#include <util/imap.h>
#include <util/smap.h>

struct ti_types_s
{
    imap_t * imap;
    smap_t * smap;
    smap_t * removed;  /* map with type id's which are removed */
    uint16_t next_id;
    ti_collection_t * collection;
};

#endif /* TI_TYPES_T_H_ */
