/*
 * ti/iter.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/iter.h>
#include <ti/iter.h>

ti_iter_t * ti_iter_create(void)
{
    ti_iter_t * iter = malloc(sizeof(ti_iter_t));
    if (!iter)
        return NULL;

    iter[0]->name = NULL;
    iter[1]->name = NULL;

    return iter;
}

void ti_iter_destroy(ti_iter_t * iter)
{
    if (!iter)
        return;
    ti_name_drop(iter[0]->name);
    ti_name_drop(iter[1]->name);
    free(iter);
}
