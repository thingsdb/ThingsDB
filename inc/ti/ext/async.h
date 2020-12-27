/*
 * ti/ext/async.h
 */
#ifndef TI_EXT_ASYNC_H_
#define TI_EXT_ASYNC_H_

typedef struct ti_ext_async_s ti_ext_async_t;

#include <inttypes.h>
#include <ti/query.t.h>
#include <ti/val.t.h>
#include <ti/ext.h>
#include <util/vec.h>


struct ti_ext_async_s
{
    ti_ext_cb cb;
    ti_ext_enum tp;
};

ti_ext_t * ti_ext_async(void);

#endif  /* TI_EXT_ASYNC_H_ */
