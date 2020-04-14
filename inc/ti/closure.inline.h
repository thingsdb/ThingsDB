/*
 * ti/closure.inline.h
 */
#ifndef TI_CLOSURE_INLINE_H_
#define TI_CLOSURE_INLINE_H_

#include <ti/closure.h>
#include <ti/query.h>
#include <ti/do.h>
#include <ex.h>

TI_INLINE(int) ti_closure_do_statement(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e)
{
    if (ti_do_statement(query, ti_closure_statement(closure), e) == EX_RETURN)
        e->nr = 0;
    return e->nr;
}

TI_INLINE(int) ti_closure_try_wse(
        ti_closure_t * closure, ti_query_t * query, ex_t * e)
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
