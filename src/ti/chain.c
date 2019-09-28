/*
 * ti/chain.c
 */
#include <ti/chain.h>
#include <tiinc.h>

void ti_chain_set(ti_chain_t * chain, ti_thing_t * thing, ti_name_t * name)
{
    ti_incref(thing);
    ti_incref(name);

    if (chain->thing)
    {
        ti_val_drop((ti_val_t *) chain->thing);
        ti_name_drop(chain->name);
    }

    chain->thing = thing;
    chain->name = name;
}

void ti_chain_move(ti_chain_t * target, ti_chain_t * source)
{
    target->thing = source->thing;
    target->name = source->name;

    source->thing = NULL;
}

void ti__chain_unset_(ti_chain_t * chain)
{
    assert (chain->thing);

    ti_val_drop((ti_val_t *) chain->thing);
    ti_name_drop(chain->name);

    chain->thing = NULL;
}

