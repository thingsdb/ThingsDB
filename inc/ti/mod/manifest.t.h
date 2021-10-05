/*
 * ti/mod/manifest.t.h
 */
#ifndef TI_MOD_MANIFEST_T_H_
#define TI_MOD_MANIFEST_T_H_

#define TI_MANIFEST "module.json"

typedef struct ti_mod_manifest_s ti_mod_manifest_t;

#include <util/vec.h>
#include <util/smap.h>

struct ti_mod_manifest_s
{
    char * main;                        /* required */
    char * version;                     /* required */
    char * doc;                         /* NULL or string */
    _Bool * load;                       /* NULL or true/false */
    uint8_t * deep;                     /* NULL or 0..127 */
    vec_t * defaults;                   /* ti_item_t */
    vec_t * includes;                   /* char *, only for installation */
    smap_t * exposes;                   /* ti_mod_expose_t */
};

#endif  /* TI_MOD_MANIFEST_T_H_ */
