/*
 * ti/varr.t.h
 */
#ifndef TI_VARR_T_H_
#define TI_VARR_T_H_

#define VARR(__x)  ((ti_varr_t *) (__x))->vec

typedef struct ti_varr_s ti_varr_t;
typedef struct ti_tuple_s ti_tuple_t;

enum
{
    TI_VARR_FLAG_TUPLE      =1<<0,      /* array is immutable; nested, and
                                            only nested array's are tuples;
                                            once a tuple is direct assigned to
                                            a thing, it converts back to a
                                            mutable list. */
    TI_VARR_FLAG_MHT        =1<<1,      /* array may-have-things; some code
                                            might skip arrays without this flag
                                            while searching for things; */
    TI_VARR_FLAG_MHR        =1<<2,      /* array may-have-rooms; some code
                                            might skip arrays without this flag
                                            while searching for rooms; */
};

#define ti_varr_may_flags(varr__) \
    ((varr__)->flags&(TI_VARR_FLAG_MHT|TI_VARR_FLAG_MHR))

#include <ex.h>
#include <inttypes.h>
#include <ti/thing.t.h>
#include <util/vec.h>

struct ti_tuple_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    int:16;
    vec_t * vec;
};

struct ti_varr_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    int:16;
    vec_t * vec;
    ti_thing_t * parent;    /* without reference,
                               NULL when this is a variable or tuple */
    void * key_;            /* ti_name_t, ti_raw_t or ti_field_t; all  without
                               reference */
};

#endif  /* TI_VARR_T_H_ */

