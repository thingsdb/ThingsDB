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

ti_closure_t * ti_closure_from_node(cleri_node_t * node);
ti_closure_t * ti_closure_from_strn(const char * str, size_t n);
void ti_closure_destroy(ti_closure_t * closure);
int ti_closure_unbound(ti_closure_t * closure);
int ti_closure_to_packer(ti_closure_t * closure, qp_packer_t ** packer);
int ti_closure_to_file(ti_closure_t * closure, FILE * f);
uchar * ti_closure_uchar(ti_closure_t * closure, size_t * n);
static inline cleri_node_t * ti_closure_scope_nd(ti_closure_t * closure);
static inline _Bool ti_closure_wse(ti_closure_t * closure);
static inline int ti_closure_try_lock(ti_closure_t * closure, ex_t * e);
static inline void ti_closure_unlock(ti_closure_t * closure);

struct ti_closure_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad16;
    cleri_node_t * node;
};

static inline _Bool ti_closure_wse(ti_closure_t * closure)
{
    return (
        closure->node->str != closure->node->data &&
        (((uintptr_t) closure->node->data) & TI_VFLAG_CLOSURE_WSE)
    );
}

static inline cleri_node_t * ti_closure_scope_nd(ti_closure_t * closure)
{
    /*  closure = Sequence('|', List(name, opt=True), '|', scope)  */
    return closure->node->children->next->next->next->node;
}

static inline int ti_closure_try_lock(ti_closure_t * closure, ex_t * e)
{
    if (closure->flags & TI_VFLAG_CLOSURE_LOCK)
    {
        ex_set(e, EX_BAD_DATA,
                "closures cannot be used recursively"TI_SEE_DOC("#closure"));
        return -1;
    }
    return (closure->flags |= TI_VFLAG_CLOSURE_LOCK) & 0;
}

/* May be called when `closure` is NULL */
static inline void ti_closure_unlock(ti_closure_t * closure)
{
    if (closure)
        closure->flags &= ~TI_VFLAG_CLOSURE_LOCK;
}


#endif  /* TI_CLOSURE_H_ */
