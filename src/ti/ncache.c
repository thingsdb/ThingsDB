/*
 * ti/ncache.c
 */
#include <assert.h>
#include <util/logger.h>
#include <ti/ncache.h>

/*
 * Assigned closures and procedures require to own a `query` string, and they
 * need to prepare all the `primitives` cached values since a query which is
 * using the closure or procedure does not have space to store these.
 */
ti_ncache_t * ti_ncache_create(char * query, size_t n)
{
    ti_ncache_t * ncache = malloc(sizeof(ti_ncache_t));
    if (!ncache)
        return NULL;

    ncache->query = query;
    ncache->nd_val_cache = vec_new(n);

    if (!ncache->nd_val_cache)
    {
        free(ncache);
        return NULL;
    }

    return ncache;
}

void ti_ncache_destroy(ti_ncache_t * ncache)
{
    if (!ncache)
        return;
    vec_destroy(ncache->nd_val_cache, (vec_destroy_cb) ti_val_drop);
    free(ncache->query);
    free(ncache);
}

int ti_ncache_gen_primitives(vec_t * vcache, cleri_node_t * node, ex_t * e)
{
    if (node->cl_obj->gid == CLERI_GID_PRIMITIVES)
    {
        node = node->children->node;
        switch (node->cl_obj->gid)
        {
        case CLERI_GID_T_FLOAT:
            assert (!node->data);
            node->data = ti_vfloat_create(strx_to_double(node->str));
            break;
        case CLERI_GID_T_INT:
            assert (!node->data);
            node->data = ti_vint_create(strx_to_int64(node->str));
            if (errno == ERANGE)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            break;
        case CLERI_GID_T_REGEX:
            assert (!node->data);
            node->data = ti_regex_from_strn(node->str, node->len, e);
            if (!node->data)
                return e->nr;
            break;
        case CLERI_GID_T_STRING:
            assert (!node->data);
            node->data = ti_raw_from_ti_string(node->str, node->len);
            break;

        default:
            return 0;
        }
        assert (vec_space(vcache));

        if (node->data)
            VEC_push(vcache, node->data);
        else
            ex_set_alloc(e);

        return e->nr;
    }

    for (cleri_children_t * child = node->children; child; child = child->next)
        if (ti_ncache_gen_primitives(vcache, child->node, e))
            return e->nr;

    return e->nr;
}
