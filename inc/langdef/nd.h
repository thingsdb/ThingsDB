/*
 * langdef/nd.h
 */
#ifndef LANGDEF_ND_H_
#define LANGDEF_ND_H_

#include <cleri/cleri.h>
#include <langdef/langdef.h>

#define LANGDEF_ND_FLAG_UNIQUE 1<<15

static inline _Bool langdef_nd_is_function(cleri_node_t * nd);
static inline _Bool langdef_nd_fun_has_zero_params(cleri_node_t * nd);
static inline _Bool langdef_nd_fun_has_one_param(cleri_node_t * nd);
static inline _Bool langdef_nd_match_str(cleri_node_t * nd, char * str);
int langdef_nd_n_function_params(cleri_node_t * nd);
void langdef_nd_flag(cleri_node_t * nd, int flags);

static inline _Bool langdef_nd_is_function(cleri_node_t * nd)
{
    return nd->cl_obj->gid == CLERI_GID_FUNCTION;
}

static inline _Bool langdef_nd_fun_has_zero_params(cleri_node_t * nd)
{
    return !nd->children;
}

static inline _Bool langdef_nd_fun_has_one_param(cleri_node_t * nd)
{
    return nd->children && !nd->children->next;
}

static inline _Bool langdef_nd_match_str(cleri_node_t * nd, char * str)
{
    return strncmp(str, nd->str, nd->len) == 0 && str[nd->len] == '\0';
}

#endif  /* LANGDEF_ND_H_ */
