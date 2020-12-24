/*
 * ti/addon.h
 */
#ifndef TI_ADDON_H_
#define TI_ADDON_H_

typedef struct ti_addon_s ti_addon_t;

#include <inttypes.h>
#include <ti/query.t.h>
#include <ti/val.t.h>
#include <util/vec.h>

typedef int (*ti_addon_cb)(ti_query_t * query, ex_t *);


struct ti_addon_s
{
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad1;
    uint32_t _pad2;
    ti_addon_cb cb;
};

#endif  /* TI_ADDON_H_ */
