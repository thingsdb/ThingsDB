/*
 * ti/modu/manifest.t.h
 */
#ifndef TI_MODU_MANIFEST_T_H_
#define TI_MODU_MANIFEST_T_H_

#define TI_MANIFEST "module.json"

typedef struct ti_modu_manifest_s ti_modu_manifest_t;

#include <util/vec.h>
#include <util/smap.h>

struct ti_modu_manifest_s
{
    _Bool is_py;                        /* true/false */
    char * main;                        /* required */
    char * version;                     /* required */
    char * doc;                         /* NULL or string */
    _Bool * load;                       /* NULL or true/false */
    uint8_t * deep;                     /* NULL or 0..127 */
    vec_t * defaults;                   /* ti_item_t */
    vec_t * includes;                   /* char *, only for installation */
    vec_t * requirements;               /* char *, pip requirements */
    smap_t * exposes;                   /* ti_modu_expose_t */
};

#endif  /* TI_MODU_MANIFEST_T_H_ */
