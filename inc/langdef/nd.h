/*
 * langdef/nd.h
 */
#ifndef LANGDEF_ND_H_
#define LANGDEF_ND_H_

#include <cleri/cleri.h>
#include <langdef/langdef.h>
#include <string.h>

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

#define langdef_nd_match_str(nd__, str__) \
    (strncmp(str__, nd__->str, nd__->len) == 0 && str__[nd__->len] == '\0')


#endif  /* LANGDEF_ND_H_ */
