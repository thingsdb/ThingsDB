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

/*
 * Run the call-back function on all items in the dict.
 *
 * Walking stops on the first callback returning a non zero value OR when a
 * memory allocation error has occurred (-1) (can happen as values
 * need to be created from keys).
 * Otherwise, the return value is the last callback result. A return value
 * of 0 means that the callback function is called on all items in the dict.
 */
int ti_dict_walk(ti_dict_t * dict, ti_dict_cb cb, void * arg)
{

}

int ti_dict_values(ti_dict_t * dict, ti_dict_values_cb cb, void * arg)
{
    
}