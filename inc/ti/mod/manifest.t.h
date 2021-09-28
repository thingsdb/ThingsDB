/*
 * ti/mod/manifest.t.h
 */
#ifndef TI_MOD_MANIFEST_T_H_
#define TI_MOD_MANIFEST_T_H_

#define TI_MANIFEST "module.json"

#include <ex.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <ti/module.t.h>
#include <util/vec.h>

typedef struct
{
    char * main;                        /* required */
    char * version;
    vec_t * defaults;
    vec_t * includes;
    vec_t * exposes;
} ti_mod_manifest_t;

#endif  /* TI_MOD_MANIFEST_T_H_ */
