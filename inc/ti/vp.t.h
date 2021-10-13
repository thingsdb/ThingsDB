/*
 * ti/vp.t.h
 */
#ifndef TI_VP_T_H_
#define TI_VP_T_H_

typedef struct ti_vp_s ti_vp_t;

#include <util/mpack.h>
#include <ti/query.t.h>

struct ti_vp_s
{
    msgpack_packer pk;
    ti_query_t * query;                 /* May be undefined;
                                         * Query must be set when the value
                                         * packer is used with `options` > 0.
                                         * Note that setting to NULL is fine
                                         * but in this case `methods` will not
                                         * be calculated.
                                         */
};

#endif  /* TI_VP_T_H_ */
