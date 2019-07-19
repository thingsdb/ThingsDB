/*
 * ti/closure.h
 */
#ifndef TI_CLOSURE_H_
#define TI_CLOSURE_H_

typedef struct ti_closure_s ti_closure_t;

#include <stdint.h>
#include <cleri/cleri.h>
#include <qpack.h>
#include <ti.h>
#include <tiinc.h>
#include <ti/val.h>
#include <ti/ex.h>
#include <ti/syntax.h>
#include <ti/query.h>

ti_closure_t * ti_closure_from_node(cleri_node_t * node);
ti_closure_t * ti_closure_from_strn(
        ti_syntax_t * syntax,
        const char * str,
        size_t n, ex_t * e);
void ti_closure_destroy(ti_closure_t * closure);
int ti_closure_unbound(ti_closure_t * closure, ex_t * e);
int ti_closure_to_packer(ti_closure_t * closure, qp_packer_t ** packer);
int ti_closure_to_file(ti_closure_t * closure, FILE * f);
uchar * ti_closure_uchar(ti_closure_t * closure, size_t * n);
int ti_closure_lock_and_use(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e);
int ti_closure_vars_val_idx(ti_closure_t * closure, ti_val_t * v, int64_t i);
void ti_closure_unlock_use(ti_closure_t * closure, ti_query_t * query);
int ti_closure_try_wse(ti_closure_t * closure, ti_query_t * query, ex_t * e);
static inline cleri_node_t * ti_closure_scope(ti_closure_t * closure);
static inline int ti_closure_try_lock(ti_closure_t * closure, ex_t * e);
static inline int ti_closure_try_lock_and_use(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e);
static inline void ti_closure_unlock(ti_closure_t * closure);

struct ti_closure_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad16;
    vec_t * vars;               /* ti_prop_t - arguments */
    cleri_node_t * node;
};

static inline cleri_node_t * ti_closure_scope(ti_closure_t * closure)
{
    /*  closure = Sequence('|', List(name, opt=True), '|', scope)  */
    return closure->node->children->next->next->next->node;
}

static inline int ti_closure_try_lock(ti_closure_t * closure, ex_t * e)
{
    if (closure->flags & TI_VFLAG_LOCK)
    {
        ex_set(e, EX_BAD_DATA,
                "closures cannot be used recursively"TI_SEE_DOC("#closure"));
        return -1;
    }
    return (closure->flags |= TI_VFLAG_LOCK) & 0;
}

/* returns 0 on a successful lock, -1 if not */
static inline int ti_closure_try_lock_and_use(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e)
{
    if (closure->flags & TI_VFLAG_LOCK)
    {
        ex_set(e, EX_BAD_DATA,
                "closures cannot be used recursively"TI_SEE_DOC("#closure"));
        return -1;
    }
    return ti_closure_lock_and_use(closure, query, e);
}

static inline void ti_closure_unlock(ti_closure_t * closure)
{
    closure->flags &= ~TI_VFLAG_LOCK;
}

#endif  /* TI_CLOSURE_H_ */
