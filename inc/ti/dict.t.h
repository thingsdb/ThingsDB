/*
 * ti/dict.t.h
 */
#ifndef TI_DICT_T_H_
#define TI_DICT_T_H_

#define DICT(__x)  ((ti_dict_t *) (__x))->vec

typedef struct ti_dict_s ti_dict_t;

enum
{
    TI_DICT_FLAG_MHT        =1<<1,      /* dict may-have-things; some code
                                            might skip dicts without this flag
                                            while searching for things; */
    TI_DICT_FLAG_MHR        =1<<2,      /* dict may-have-rooms; some code
                                            might skip dicts without this flag
                                            while searching for rooms; */
};

#define ti_dict_may_flags(dict__) \
    ((dict__)->flags&(TI_DICT_FLAG_MHT|TI_DICT_FLAG_MHR))

#include <ex.h>
#include <inttypes.h>
#include <ti/thing.t.h>
#include <util/imap.h>
#include <util/smap.h>
#include <util/umap.h>

/* parent and key location equal to  varr, vset and dict */
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

