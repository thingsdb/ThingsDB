
/*
 * ti/closure.t.h
 */
#ifndef TI_CLOSURE_T_H_
#define TI_CLOSURE_T_H_

#define TI_CLOSURE_MAX_RECURSION_DEPTH 24

typedef struct ti_closure_s ti_closure_t;

#include <cleri/cleri.h>
#include <inttypes.h>
#include <util/vec.h>

typedef struct
{
    uint32_t pos;
    uint32_t prev_block_stack;
} ti_stacked_t;

/*
 * Reserve 1024 bytes for a closure
 */
struct ti_closure_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t depth;             /* recursion depth */
    vec_t * vars;               /* ti_prop_t - arguments */
    vec_t * stacked;            /* ti_val_t - stacked values */
    cleri_node_t * node;
    ti_stacked_t stack_pos[TI_CLOSURE_MAX_RECURSION_DEPTH];
};

#endif  /* TI_CLOSURE_T_H_ */
