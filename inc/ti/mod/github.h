/*
 * ti/mod/github.h
 */
#ifndef TI_MOD_GITHUB_H_
#define TI_MOD_GITHUB_H_

#include <ex.h>

typedef struct ti_mod_github_s ti_mod_github_t;

_Bool ti_mod_github_test(const char * s, size_t n);
ti_mod_github_t * ti_mod_github_create(const char * s, size_t n);

struct ti_mod_github_s
{
    char * owner;
    char * repo;
    char * token;  /* NULL when public, not visible */
    char * ref;  /* tag, branch etc. */
};


#endif  /* TI_MOD_GITHUB_H_ */
