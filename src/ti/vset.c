/*
 * ti/vset.c
 */
#include <assert.h>
#include <tiinc.h>
#include <stdlib.h>
#include <ti/vset.h>
#include <ti/val.h>
#include <util/logger.h>

ti_vset_t * ti_vset_create(size_t sz)
{
    ti_vset_t * vset = malloc(sizeof(ti_vset_t));
    if (!vset)
        return NULL;

    vset->ref = 1;
    vset->tp = TI_VAL_SET;

    vset->imap = imap_new(sz);
    if (!vset->imap)
    {
        free(vset);
        return NULL;
    }

    return vset;
}

void ti_vset_destroy(ti_vset_t * vset)
{
    if (!vset)
        return;
    imap_destroy(vset->imap, (imap_destroy_cb) ti_val_drop);
    free(vset);
}



