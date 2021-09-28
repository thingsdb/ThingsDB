/*
 * ti/mod/github.h
 */
#ifndef TI_MOD_GITHUB_H_
#define TI_MOD_GITHUB_H_

#include <ex.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <ti/module.t.h>
#include <ti/mod/github.t.h>

_Bool ti_mod_github_test(const char * s, size_t n);
ti_mod_github_t * ti_mod_github_create(const char * s, size_t n, ex_t * e);
void ti_mod_github_destroy(ti_mod_github_t * gh);
void ti_mod_github_install(ti_module_t * module);
const char * ti_mod_github_code_str(ti_mod_github_t * gh);


#endif  /* TI_MOD_GITHUB_H_ */
