/*
 * ti/ext.h
 */
#ifndef TI_EXT_H_
#define TI_EXT_H_

typedef struct ti_ext_s ti_ext_t;

#include <inttypes.h>
#include <ti/future.t.h>
#include <ti/query.t.h>
#include <ti/val.t.h>
#include <util/vec.h>

typedef int (*ti_ext_cb)(ti_future_t * future);

typedef enum
{
    TI_EXT_ASYNC
} ti_ext_enum;

struct ti_ext_s
{
    ti_ext_cb cb;
    ti_ext_enum tp;
};

#endif  /* TI_ADDON_H_ */
