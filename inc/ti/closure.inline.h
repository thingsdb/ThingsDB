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
     *
     * bug #259, we used to decrement the local stack with `closure->vars->n`
     * but this would require a GC drop on each iteration of the closure vars.
     * Instead, we can adjust the local_stack and do not count the closure
     * vars as local scope.
     */
    query->local_stack = n;

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
    if (!query->change && ((closure->flags & (
                TI_CLOSURE_FLAG_BTSCOPE|
                TI_CLOSURE_FLAG_BCSCOPE|
                TI_CLOSURE_FLAG_WSE
            )) == TI_CLOSURE_FLAG_WSE))
    {
        ex_set(e, EX_OPERATION,
            "closures with side effects require a change but none is created; "
            "use `wse(...)` to enforce a change"DOC_WSE);
        return -1;
    }
    return 0;
}

static inline int ti_closure_inc_future(ti_closure_t * closure, ex_t * e)
{
    if (closure->future_count == TI_CLOSURE_MAX_FUTURE_COUNT)
        ex_set(e, EX_OPERATION,
                "maximum nested future count has been reached"DOC_CLOSURE);
    else
        closure->future_count++;
    return e->nr;
}

static inline void ti_closure_dec_future(ti_closure_t * closure)
{
    assert(closure->future_count);
    --closure->future_count;
}

static inline void ti_closure_unsafe_drop(ti_closure_t * closure)
{
    if (!--closure->ref)
        ti_closure_destroy(closure);
}

static inline void ti_closure_drop(ti_closure_t * closure)
{
    if (closure && !--closure->ref)
        ti_closure_destroy(closure);
}

#endif  /* TI_CLOSURE_INLINE_H_ */
