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
    ti_query_t * query;                 /* may be undefined */
};

#endif  /* TI_VP_T_H_ */
