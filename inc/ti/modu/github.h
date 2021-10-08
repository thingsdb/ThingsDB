/*
 * ti/modu/github.h
 */
#ifndef TI_MODU_GITHUB_H_
#define TI_MODU_GITHUB_H_

#include <ex.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <ti/modu/github.t.h>
#include <uv.h>

_Bool ti_modu_github_test(const char * s, size_t n);
int ti_modu_github_init(
        ti_modu_github_t * gh,
        const char * s,
        size_t n,
        ex_t * e);
ti_modu_github_t * ti_modu_github_create(const char * s, size_t n, ex_t * e);
void ti_modu_github_download(uv_work_t * work);
void ti_modu_github_manifest(uv_work_t * work);
void ti_modu_github_destroy(ti_modu_github_t * gh);
const char * ti_modu_github_code_str(ti_modu_github_t * gh);


#endif  /* TI_MODU_GITHUB_H_ */
