/*
 * ti/chain.h
 */
#ifndef TI_CHAIN_H_
#define TI_CHAIN_H_

typedef struct ti_chain_s ti_chain_t;

#include <assert.h>
#include <stdlib.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ex.h>

void ti_chain_set(ti_chain_t * chain, ti_thing_t * thing, ti_name_t * name);
void ti_chain_move(ti_chain_t * target, ti_chain_t * source);
void ti__chain_unset_(ti_chain_t * chain);
static inline void ti_chain_init(ti_chain_t * chain);
static inline void ti_chain_unset(ti_chain_t * chain);
static inline _Bool ti_chain_is_set(ti_chain_t * chain);

struct ti_chain_s
{
    ti_thing_t * thing;         /* with reference */
    ti_name_t * name;           /* with reference */
};

static inline void ti_chain_init(ti_chain_t * chain)
{
    chain->thing = NULL;
    chain->name = NULL;
}

static inline void ti_chain_unset(ti_chain_t * chain)
{
    if (chain->thing)
        ti__chain_unset_(chain);
}

static inline _Bool ti_chain_is_set(ti_chain_t * chain)
{
    return !!chain->thing;
}

#endif  /* TI_CHAIN_H_ */
