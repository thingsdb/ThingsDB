/*
 * langdef/nd.h
 */
#ifndef LANGDEF_ND_H_
#define LANGDEF_ND_H_

#include <cleri/cleri.h>
#include <langdef/langdef.h>
#include <string.h>

static inline _Bool langdef_nd_fun_has_zero_params(cleri_node_t * nd);
static inline _Bool langdef_nd_fun_has_one_param(cleri_node_t * nd);
static inline int langdef_nd_n_function_params(cleri_node_t * nd);
static inline _Bool langdef_nd_match_str(cleri_node_t * nd, char * str);

static inline _Bool langdef_nd_fun_has_zero_params(cleri_node_t * nd)
{
    return !nd->children;
}

static inline _Bool langdef_nd_fun_has_one_param(cleri_node_t * nd)
{
    return ((intptr_t) nd->data) == 1;
}

static inline int langdef_nd_n_function_params(cleri_node_t * nd)
{
    return (int) ((intptr_t) nd->data);
}

static inline _Bool langdef_nd_match_str(cleri_node_t * nd, char * str)
{
    return strncmp(str, nd->str, nd->len) == 0 && str[nd->len] == '\0';
}

#endif  /* LANGDEF_ND_H_ */
