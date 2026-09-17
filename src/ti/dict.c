/*
 * ti/dict.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/dict.h>
#include <tiinc.h>
#include <util/logger.h>


ti_dict_t * ti_dict_create(size_t sz)
{
    ti_dict_t * dict = malloc(sizeof(ti_dict_t));
    if (!dict)
        return NULL;

    dict->ref = 1;
    dict->tp = TI_VAL_DICT;
    dict->flags = 0;
    dict->n = 0;

    dict->imap_ = NULL;
    dict->smap_ = NULL;
    dict->umap_ = NULL;

    dict->parent = NULL;

    return dict;
}

void ti_dict_destroy(ti_dict_t * dict)
{
    imap_destroy(dict->imap_, (imap_destroy_cb) ti_val_unsafe_gc_drop);
    smap_destroy(dict->smap_, (smap_destroy_cb) ti_val_unsafe_gc_drop);
    umap_destroy(dict->umap_, (umap_destroy_cb) ti_val_unsafe_gc_drop);
    free(dict);
}