/*
 * val.h
 */
#ifndef TI_VAL_H_
#define TI_VAL_H_

typedef enum
{
    TI_VAL_NIL,
    TI_VAL_INT,
    TI_VAL_FLOAT,
    TI_VAL_BOOL,
    TI_VAL_RAW,
    TI_VAL_PRIMITIVES,
    TI_VAL_ELEM,
    TI_VAL_ELEMS,
} ti_val_e;
typedef struct ti_val_s ti_val_t;
typedef union ti_val_u ti_val_via_t;

#include <qpack.h>
#include <stdint.h>
#include <ti/raw.h>
#include <ti/thing.h>
#include <util/vec.h>

ti_val_t * ti_val_create(ti_val_e tp, void * v);
ti_val_t * ti_val_weak_create(ti_val_e tp, void * v);
void ti_val_destroy(ti_val_t * val);
void ti_val_weak_set(ti_val_t * val, ti_val_e tp, void * v);
int ti_val_set(ti_val_t * val, ti_val_e tp, void * v);
void ti_val_clear(ti_val_t * val);
int ti_val_to_packer(ti_val_t * val, qp_packer_t * packer);
int ti_val_to_file(ti_val_t * val, FILE * f);

union ti_val_u
{
    void * nil_;
    int64_t int_;
    double float_;
    _Bool bool_;
    ti_raw_t * raw_;
    vec_t * primitives_;
    ti_thing_t * thing_;
    vec_t * things_;
};

struct ti_val_s
{
    ti_val_e tp;
    ti_val_via_t via;
};

#endif /* TI_VAL_H_ */
