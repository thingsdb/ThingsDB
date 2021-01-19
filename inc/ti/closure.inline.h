/*
 * ti/closure.inline.h
 */
#ifndef TI_CLOSURE_INLINE_H_
#define TI_CLOSURE_INLINE_H_

#include <doc.h>
#include <ex.h>
#include <ti/closure.h>
#include <ti/do.h>
#include <ti/prop.h>
#include <ti/query.h>

static inline int ti_closure_do_statement(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e)
{
    uint32_t prev_local_stack = query->local_stack;
    uint32_t n = query->vars->n;
    /*
     * Keep the "position" in the variable stack so we can later break down
     * all used variable inside the closure body.
     */
    query->local_stack = n - closure->vars->n;

    if (ti_do_statement(query, ti_closure_statement(closure), e) == EX_RETURN)
        e->nr = 0;

    /*
     * Break down all used variables inside the closure body. Make sure we mark
     * things for garbage collection if their reference has not reached zero.
     */
    while (query->vars->n > n)
        ti_prop_destroy(VEC_pop(query->vars));

    /*
     * Restore the previous stack "position" so the optional parent body
     * has the correct stack to work with.
     */
    query->local_stack = prev_local_stack;
    return e->nr;
}

static inline int ti_closure_try_wse(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e)
{
    /*
     * The current implementation never sets the `WSE` flag when bound to a
     * scope, but nevertheless we still check explicit so this still works
     * if we later decide to change the code.
     */
    if (    ((closure->flags & (
                TI_VFLAG_CLOSURE_BTSCOPE|
                TI_VFLAG_CLOSURE_BCSCOPE|
                TI_VFLAG_CLOSURE_WSE
            )) == TI_VFLAG_CLOSURE_WSE) &&
            (~query->flags & TI_QUERY_FLAG_WSE))
    {
        ex_set(e, EX_OPERATION,
                "stored closures with side effects must be "
                "wrapped using `wse(...)`"DOC_WSE);
        return -1;
    }
    return 0;
}

static inline int ti_closure_inc_future(ti_closure_t * closure, ex_t * e)
{
    if (closure->future_depth == TI_CLOSURE_MAX_FUTURE_RECURSION_DEPTH)
        ex_set(e, EX_OPERATION,
                "maximum recursion depth exceeded"DOC_CLOSURE);
    else
        closure->future_depth++;
    return e->nr;
}

static inline void ti_closure_dec_future(ti_closure_t * closure)
{
    assert(closure->future_depth);
    --closure->future_depth;
}

#endif  /* TI_CLOSURE_INLINE_H_ */
