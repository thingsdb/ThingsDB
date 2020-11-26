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
    uint32_t prev_block_stack = query->block_stack;
    uint32_t n = query->vars->n;
    /*
     * Keep the "position" in the variable stack so we can later break down
     * all used variable inside the block.
     */
    query->block_stack = n - closure->vars->n;

    if (ti_do_statement(query, ti_closure_statement(closure), e) == EX_RETURN)
        e->nr = 0;

    /*
     * Break down all used variables inside the block. Make sure we mark
     * things for garbage collection if their reference has not reached zero.
     */
    while (query->vars->n > n)
        ti_prop_destroy(VEC_pop(query->vars));

    /*
     * Restore the previous stack "position" so the optional parent block
     * has the correct block_stack to work with.
     */
    query->block_stack = prev_block_stack;
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
            (~query->qbind.flags & TI_QBIND_FLAG_WSE))
    {
        ex_set(e, EX_OPERATION_ERROR,
                "stored closures with side effects must be "
                "wrapped using `wse(...)`"DOC_WSE);
        return -1;
    }
    return 0;
}

#endif  /* TI_CLOSURE_INLINE_H_ */
