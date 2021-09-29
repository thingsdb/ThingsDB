/*
 * ti/mod/expose.c
 */
#include <ti/mod/expose.h>
#include <util/logger.h>

void ti_mod_expose_destroy(ti_mod_expose_t * expose)
{
    /* TODO: free ... */
    free(expose);
}

int ti_mod_expose_call(
        ti_module_t * module,
        ti_mod_expose_t * expose,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    LOGC("mod: %p expose: %p, q: %p, nd: %p, e: %p",
            module, expose, query, nd, e);
    return 0;
}
