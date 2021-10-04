/*
 * ti/mod/expose.c
 */
#include <ti/mod/expose.h>
#include <util/logger.h>

ti_mod_expose_t * ti_mod_expose_create(void)
{
    ti_mod_expose_t * expose = calloc(1, sizeof(ti_mod_expose_t));
    return expose;
}

void ti_mod_expose_destroy(ti_mod_expose_t * expose)
{
    /* TODO: free ... */
    if (!expose)
        return;
    free(expose->doc);
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

int ti_mod_expose_info_to_pk(
        const char * key,
        size_t n,
        ti_mod_expose_t * expose,
        msgpack_packer * pk)
{
    size_t defaults_n = (expose->defaults ? expose->defaults->n : 0) + \
            !!expose->deep + \
            !!expose->load;
    size_t sz = 1 + !!expose->argmap + !!expose->doc + !!(defaults_n);

    int rc = -(
            msgpack_pack_map(pk, sz) ||

            mp_pack_str(pk, "name") ||
            mp_pack_strn(pk, key, n) ||

            mp_pack_str(pk, "doc") ||
            mp_pack_str(pk, expose->doc)
    );
    /* TODO pack more... */
    return rc;
}
