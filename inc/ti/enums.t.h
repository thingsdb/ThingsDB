/*
 * ti/enums.t.h
 */
#ifndef TI_ENUMS_T_H_
#define TI_ENUMS_T_H_

typedef struct ti_enums_s ti_enums_t;

#include <ti/collection.t.h>
#include <util/imap.h>
#include <util/smap.h>

struct ti_enums_s
{
    imap_t * imap;
    smap_t * smap;
    ti_collection_t * collection;
};

#endif /* TI_ENUMS_T_H_ */
