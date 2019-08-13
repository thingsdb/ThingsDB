/*
 * ti/chained.c
 */
#include <ti/chained.h>
#include <tiinc.h>

ti_chained_t * ti_chained_create(int sz)
{
    ti_chained_t * chained = malloc(
            sizeof(ti_chained_t) + (sz * sizeof(ti_chain_t)));

    if (!chained)
        return NULL;

    chained->current = -1;
    chained->sz = sz;
    chained->n = 0;

    return chained;
}

int ti_chained_append(
        ti_chained_t ** chainedaddr,
        ti_thing_t * thing,
        ti_name_t * name)
{
    ti_chain_t * chain;
    ti_chained_t * chained = *chainedaddr;

    if (chained->n == chained->sz)
    {
        ti_chained_t * tmp;
        int sz = chained->sz;

        if (sz < 2)
            chained->sz++;
        else if (sz < 32)
            chained->sz *= 2;
        else
            chained->sz += 32;

        tmp = realloc(
                chained,
                sizeof(ti_chained_t) + chained->sz * sizeof(ti_chain_t));

        if (!tmp)
        {
            /* restore original size */
            chained->sz = sz;
            return -1;
        }

        *chainedaddr = chained = tmp;
    }

    chained->current = chained->n;

    chain = &chained->chain[chained->n++];
    chain->thing = thing;
    chain->name = name;

    ti_incref(thing);
    ti_incref(name);

    return 0;
}

_Bool ti_chained_thing_in_use(ti_chained_t * chained, ti_thing_t * thing)
{
    for (int i = 0; i < chained->n; ++i)
        if (chained->chain[i].thing == thing)
            return true;
    return false;
}

void ti__chained_leave_(ti_chained_t * chained, int until)
{
    assert (until >= 0);
    assert (chained->n > until);

    ti_chain_t * chain;
    do
    {
        chain = &chained->chain[--chained->n];
        ti_val_drop((ti_val_t *) chain->thing);
        ti_name_drop(chain->name);
    }
    while (chained->n > until);
}
