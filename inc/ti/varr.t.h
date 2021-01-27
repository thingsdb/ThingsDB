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
};

#include <ex.h>
#include <inttypes.h>
#include <ti/name.t.h>
#include <ti/thing.t.h>
#include <util/vec.h>

struct ti_tuple_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t spec;
    vec_t * vec;
};

struct ti_varr_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t spec;
    vec_t * vec;
    ti_thing_t * parent;    /* without reference,
                               NULL when this is a variable or tuple */
    ti_raw_t * key;         /* without reference */
};

#endif  /* TI_VARR_T_H_ */

