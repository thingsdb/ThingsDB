/*
 * ti/dict.t.h
 */
#ifndef TI_DICT_T_H_
#define TI_DICT_T_H_

#define DICT(__x)  ((ti_dict_t *) (__x))->vec

typedef struct ti_dict_s ti_dict_t;

#include <ex.h>
#include <inttypes.h>
#include <ti/thing.t.h>
#include <util/imap.h>
#include <util/smap.h>
#include <util/umap.h>

/* Implements ti_parent_t */
struct ti_dict_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    int:16;
    ti_thing_t * parent;    /* without reference,
                               NULL when this is a variable */
    void * key_;            /* ti_name_t, ti_raw_t or ti_field_t; all without
                               reference */
    size_t n;
    imap_t * imap_;
    smap_t * smap_;
    umap_t * umap_;
};

#endif  /* TI_DICT_T_H_ */

