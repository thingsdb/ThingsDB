/*
 * ti/chained.h
 */
#ifndef TI_CHAINEND_H_
#define TI_CHAINEND_H_

typedef struct ti_chained_s ti_chained_t;
typedef struct ti_chain_s ti_chain_t;

#include <assert.h>
#include <stdlib.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/ex.h>

ti_chained_t * ti_chained_create(int sz);
int ti_chained_append(
        ti_chained_t ** chainedaddr,
        ti_thing_t * thing,
        ti_name_t * name);
_Bool ti__chained_in_use_(
        ti_chained_t * chained,
        ti_chain_t * chain,
        ex_t * e);
static inline _Bool ti_chained_in_use(
        ti_chained_t * chained,
        ti_chain_t * chain,
        ex_t * e);
static inline void ti_chained_destroy(ti_chained_t * chained);
static inline ti_chain_t * ti_chained_get(ti_chained_t * chained);
static inline void ti_chained_null(ti_chained_t * chained);
static inline void ti_chained_leave(ti_chained_t * chained, int until);

struct ti_chain_s
{
    ti_thing_t * thing;         /* ? reference */
    ti_name_t * name;           /* ? reference */
};

struct ti_chained_s
{
    int current;
    int sz;
    int n;
    ti_chain_t chain[];
};

static inline _Bool ti_chained_in_use(
        ti_chained_t * chained,
        ti_chain_t * chain,
        ex_t * e)
{
    return chain ? ti__chained_in_use_(chained, chain, e) : false;
}

static inline void ti_chained_destroy(ti_chained_t * chained)
{
    assert(chained->n == 0);
    free(chained);
}

static inline ti_chain_t * ti_chained_get(ti_chained_t * chained)
{
    assert (chained->current == -1 || chained->current == chained->n -1);
    return chained->current < 0 ? NULL : chained->chain[chained->current];
}

static inline void ti_chained_leave(ti_chained_t * chained, int until)
{
    if (chained->n > until)
        ti__chained_leave_(chained, until);
}

static inline void ti_chained_null(ti_chained_t * chained)
{
    chained->current = -1;
}

#endif  /* TI_CHAINEND_H_ */
