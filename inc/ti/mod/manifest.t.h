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
    char * doc;
    _Bool * load;
    uint8_t * deep;
    vec_t * defaults;
    vec_t * includes;                   /* only used during installation */
    smap_t * exposes;
};

#endif  /* TI_MOD_MANIFEST_T_H_ */
