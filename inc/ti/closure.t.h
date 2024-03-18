
/*
 * ti/closure.t.h
 */
#ifndef TI_CLOSURE_T_H_
#define TI_CLOSURE_T_H_

#define TI_CLOSURE_MAX_RECURSION_DEPTH 24
#define TI_CLOSURE_MAX_FUTURE_COUNT 255

typedef struct ti_closure_s ti_closure_t;

enum
{
    TI_CLOSURE_FLAG_BTSCOPE =1<<0,      /* closure is bound to query string
                                            within the thingsdb scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string.*/
    TI_CLOSURE_FLAG_BCSCOPE =1<<1,     /* closure is bound to query string
                                            within a collection scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string. */
    TI_CLOSURE_FLAG_WSE     =1<<2,     /* stored closure with side effects;
                                            when closure may make changes they
                                            require a change. Must be checked
                                            for stored closures since only then
                                            is is possible no change is created
                                            */
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
    uint8_t future_count;       /* future count, every time the future is used
                                   by `then`, `else` or within a future, this
                                   counter goes up.
                                   max defined by TI_CLOSURE_MAX_FUTURE_COUNT
                                 */
    vec_t * vars;               /* ti_prop_t - arguments */
    vec_t * stacked;            /* ti_val_t - stacked values */
    cleri_node_t * node;
    uint32_t stack_pos[TI_CLOSURE_MAX_RECURSION_DEPTH];
};

#endif  /* TI_CLOSURE_T_H_ */
