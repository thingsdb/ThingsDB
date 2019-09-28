
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
#include <doc.h>
#include <tiinc.h>
#include <ti/val.h>
#include <ex.h>
#include <ti/syntax.h>
#include <ti/query.h>
#include <ti/do.h>

//int ti_do_statement(ti_query_t * query, cleri_node_t * nd, ex_t * e);

ti_closure_t * ti_closure_from_node(cleri_node_t * node, uint8_t flags);
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
int ti_closure_vars_nameval(
        ti_closure_t * closure,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e);
int ti_closure_vars_val_idx(ti_closure_t * closure, ti_val_t * v, int64_t i);
void ti_closure_unlock_use(ti_closure_t * closure, ti_query_t * query);
int ti_closure_try_wse(ti_closure_t * closure, ti_query_t * query, ex_t * e);
int ti_closure_call(
        ti_closure_t * closure,
        ti_query_t * query,
        vec_t * args,
        ex_t * e);
ti_raw_t * ti_closure_doc(ti_closure_t * closure);

struct ti_closure_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad16;
    uint32_t locked_n;
    uint32_t _pad32;
    vec_t * vars;               /* ti_prop_t - arguments */
    cleri_node_t * node;
};

static inline int ti_closure_vars_prop(
        ti_closure_t * closure,
        ti_prop_t * prop,
        ex_t * e)
{
    return ti_closure_vars_nameval(closure, prop->name, prop->val, e);
}

static inline cleri_node_t * ti_closure_statement(ti_closure_t * closure)
{
    /*  closure = Sequence('|', List(name, opt=True), '|', statement)  */
    return closure->node->children->next->next->next->node;
}

static inline int ti_closure_try_lock(ti_closure_t * closure, ex_t * e)
{
    if (closure->flags & TI_VFLAG_LOCK)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "closures cannot be used recursively"DOC_CLOSURE);
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
        ex_set(e, EX_OPERATION_ERROR,
                "closures cannot be used recursively"DOC_CLOSURE);
        return -1;
    }
    return ti_closure_lock_and_use(closure, query, e);
}

static inline void ti_closure_unlock(ti_closure_t * closure)
{
    closure->flags &= ~TI_VFLAG_LOCK;
}

#endif  /* TI_CLOSURE_H_ */
