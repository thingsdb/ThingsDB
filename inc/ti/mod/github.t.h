/*
 * ti/mod/github.t.h
 */
#ifndef TI_MOD_GITHUB_T_H_
#define TI_MOD_GITHUB_T_H_

#include <ex.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <ti/module.t.h>
#include <ti/mod/manifest.t.h>

typedef enum
{
    GH_OK,
    GH_INTERNAL_ERROR,
    GH_CURLE_MODULE_DOWNLOAD_ERROR,
    GH_MANIFEST_ERROR,
} ti_mod_github_code_t;

typedef struct
{
    char * owner;
    char * repo;
    char * token;   /* NULL when public, not visible */
    char * ref;     /* tag, branch etc. */
    ti_mod_manifest_t download_manifest;      /* temporary manifest */
} ti_mod_github_t;

#endif  /* TI_MOD_GITHUB_T_H_ */
