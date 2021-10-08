/*
 * ti/modu/github.t.h
 */
#ifndef TI_MODU_GITHUB_T_H_
#define TI_MODU_GITHUB_T_H_

typedef struct ti_modu_github_s ti_modu_github_t;

#include <ex.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <ti/module.t.h>
#include <ti/modu/manifest.t.h>

struct ti_modu_github_s
{
    char * owner;
    char * repo;
    char * ref;             /* NULL or tag, branch etc. */
    char * token_header;    /* NULL when public, not visible */
    char * manifest_url;
};

#endif  /* TI_MODU_GITHUB_T_H_ */
