/*
 * ti/vup.h
 */
#ifndef TI_VUP_H_
#define TI_VUP_H_


typedef struct ti_vup_s ti_vup_t;

#include <util/mpack.h>
#include <ti/collection.h>

struct ti_vup_s
{
    ti_collection_t * collection;
    mp_unp_t * up;
    _Bool isclient;
};


#endif  /* TI_VUP_H_ */
