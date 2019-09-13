/*
 * ti/closurei.h
 */
#include <ti/closure.h>
#include <ti/query.h>
#include <ti/do.h>
#include <ex.h>

static inline int ti_closure_do_statement(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e)
{
    if (ti_do_statement(query, ti_closure_statement(closure), e) == EX_RETURN)
        e->nr = 0;
    return e->nr;
}
