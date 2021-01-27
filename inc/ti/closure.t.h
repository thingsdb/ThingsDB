
/*
 * ti/closure.t.h
 */
#ifndef TI_CLOSURE_T_H_
#define TI_CLOSURE_T_H_

#define TI_CLOSURE_MAX_RECURSION_DEPTH 24
#define TI_CLOSURE_MAX_FUTURE_RECURSION_DEPTH 1

typedef struct ti_closure_s ti_closure_t;

enum
{
    TI_CLOSURE_FLAG_BTSCOPE =1<<0,      /* closure bound to query string
                                            within the thingsdb scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string.*/
    TI_CLOSURE_FLAG_BCSCOPE =1<<1,     /* closure bound to query string
                                            within a collection scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string. */
    TI_CLOSURE_FLAG_WSE     =1<<2,     /* stored closure with side effects;
                                            when closure make changes they
                                            require an event and thus must be
                                            wrapped by wse() so we can know
                                            an event is created.
                                            (only stored closures) */
};

#include <cleri/cleri.h>
#include <inttypes.h>
#include <util/vec.h>

/*
 * Reserve 1024 bytes for a closure
 */
struct ti_closure_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint8_t depth;              /* recursion depth */
    uint8_t future_depth;       /* future recursion depth */
    vec_t * vars;               /* ti_prop_t - arguments */
    vec_t * stacked;            /* ti_val_t - stacked values */
    cleri_node_t * node;
    uint32_t stack_pos[TI_CLOSURE_MAX_RECURSION_DEPTH];
};

#endif  /* TI_CLOSURE_T_H_ */
