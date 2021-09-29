/*
 * ti/mod/github.t.h
 */
#ifndef TI_MOD_GITHUB_T_H_
#define TI_MOD_GITHUB_T_H_

typedef struct ti_mod_github_s ti_mod_github_t;

#include <ex.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <ti/module.t.h>
#include <ti/mod/manifest.t.h>

struct ti_mod_github_s
{
    char * owner;
    char * repo;
    char * ref;             /* tag, branch etc. */
    char * token_header;    /* NULL when public, not visible */
    char * manifest_url;
};

#endif  /* TI_MOD_GITHUB_T_H_ */
