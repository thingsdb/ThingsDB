/*
 * ti/addons/async.h
 */
#ifndef TI_ADDONS_ASYNC_H_
#define TI_ADDONS_ASYNC_H_

typedef struct ti_addons_async_s ti_addons_async_t;

#include <inttypes.h>
#include <ti/query.t.h>
#include <ti/val.t.h>
#include <util/vec.h>

typedef int (*ti_addon_cb)(ti_query_t * query, ex_t *);


struct ti_addons_async_t
{
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad1;
    uint32_t _pad2;
    ti_addon_cb cb;
};

#endif  /* TI_ADDONS_ASYNC_H_ */
