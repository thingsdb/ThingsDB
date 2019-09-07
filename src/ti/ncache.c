/*
 * ti/ncache.c
 */
#include <assert.h>
#include <util/logger.h>
#include <util/strx.h>
#include <ti/ncache.h>
#include <ti/val.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/names.h>
#include <ti/regex.h>
#include <ti/raw.h>
#include <ti/closure.h>
#include <langdef/langdef.h>

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
    ncache->val_cache = vec_new(n);

    if (!ncache->val_cache)
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
    vec_destroy(ncache->val_cache, (vec_destroy_cb) ti_val_drop);
    free(ncache->query);
    free(ncache);
}

static inline int ncache__i(
        ti_syntax_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE ||
            nd->cl_obj->gid == CLERI_GID_CHAIN);

    if (/* skip . and !, goto choice */
        ti_ncache_gen_node_data(syntax, vcache, nd->children->next->node, e) ||
        /* index */
        (nd->children->next->next->node->children &&
                ti_ncache_gen_node_data(
                        syntax,
                        vcache,
                        nd->children->next->next->node,
                        e)))
        return e->nr;

    /* chain */
    return nd->children->next->next->next
            ? ncache__i(
                syntax,
                vcache,
                nd->children->next->next->next->node,
                e)
            : e->nr;
}

int ncache__gen_immutable(
        ti_syntax_t * syntax,
        vec_t * vcache,
        cleri_node_t * node,
        ex_t * e)
{
    assert (node->cl_obj->gid == CLERI_GID_IMMUTABLE);

    node = node->children->node;
    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_CLOSURE:
        assert (!node->data);
        node->data = ti_closure_from_node(
                node,
                (syntax->flags & TI_SYNTAX_FLAG_THINGSDB)
                            ? TI_VFLAG_CLOSURE_BTSCOPE
                            : TI_VFLAG_CLOSURE_BCSCOPE);
        if (ncache__i(
                    syntax,
                    vcache,
                    node->children->next->next->next->node,
                    e))
            return e->nr;
        break;
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
        ex_set_mem(e);

    return e->nr;
}

int ncache__gen_name(vec_t * vcache, cleri_node_t * node, ex_t * e)
{
    assert (node->cl_obj->gid == CLERI_GID_NAME ||
            node->cl_obj->gid == CLERI_GID_VAR);
    assert (vec_space(vcache));

    node->data = ti_names_get(node->str, node->len);

    if (node->data)
        VEC_push(vcache, node->data);
    else
        ex_set_mem(e);
    return e->nr;
}

static inline int ncache__list(
        ti_syntax_t * syntax,
        vec_t * vcache,
        cleri_children_t * child,
        ex_t * e)
{
    for (; child; child = child->next->next)
        if (ncache__i(syntax, vcache, child->node, e) || !child->next)
            break;
    return e->nr;
}

static int ncache__opr(
        ti_syntax_t * syntax,
        vec_t * vcache,
        cleri_node_t * node,
        ex_t * e)
{
    assert(node->cl_obj->tp == CLERI_TP_PRIO);

    node = node->children->node;

    assert (node->cl_obj->gid == CLERI_GID_SCOPE ||
            node->cl_obj->tp == CLERI_TP_SEQUENCE);

    return node->cl_obj->gid == CLERI_GID_SCOPE
        ? ncache__i(syntax, vcache, node, e)
        : (
            ncache__opr(syntax, vcache, node->children->node, e) ||
            ncache__opr(syntax, vcache, node->children->next->next->node, e)
        );
}

static int ncache__index(
        ti_syntax_t * syntax,
        vec_t * vcache,
        cleri_node_t * node,
        ex_t * e)
{
    cleri_children_t * child = node        /* sequence */
            ->children->next->node         /* slice */
            ->children;

    if (node->children->next->next->next &&
        ncache__i(syntax, vcache, node                  /* sequence */
            ->children->next->next->next->node          /* assignment */
            ->children->next->node, e))                 /* scope */
        return e->nr;

    for (; child; child = child->next)
        if (    child->node->cl_obj->gid == CLERI_GID_SCOPE &&
                ncache__i(syntax, vcache, child->node, e))
            return e->nr;

    return e->nr;
}

/* return 0 on success, but not not always the current e->nr */
int ti_ncache_gen_node_data(
        ti_syntax_t * syntax,
        vec_t * vcache,
        cleri_node_t * node,
        ex_t * e)
{
    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        return ncache__list(
                syntax,
                vcache,
                node->children->next->node->children,
                e);
    case CLERI_GID_BLOCK:
        return ncache__list(
                syntax,
                vcache,
                node->children->next->next->node->children,
                e);
    case CLERI_GID_CHAIN:
    case CLERI_GID_SCOPE:
        return ncache__i(syntax, vcache, node, e);
    case CLERI_GID_THING:
    {
        cleri_children_t * child = node         /* sequence */
                ->children->next->node          /* list */
                ->children;
        for (; child; child = child->next->next)
        {
            /* sequence(name: scope) (only investigate the scopes */
            if (ncache__i(
                    syntax,
                    vcache,
                    child->node->children->next->next->node,
                    e))
                return e->nr;

            if (!child->next)
                break;
        }
        return e->nr;
    }
    case CLERI_GID_THING_BY_ID:
        node->data = ti_vint_create(strtoll(node->str + 1, NULL, 10));
        if (!node->data)
            ex_set_mem(e);
        return e->nr;
    case CLERI_GID_NAME_OPT_FUNC_ASSIGN:
    case CLERI_GID_VAR_OPT_FUNC_ASSIGN:
        if (!node->children->next)
            return ncache__gen_name(vcache, node->children->node, e);

        switch (node->children->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            return ncache__list(
                    syntax,
                    vcache,
                    node->children->next->node->children->next->node->children,
                    e);
        case CLERI_GID_ASSIGN:
            return (ncache__i(
                    syntax,
                    vcache,
                    node->children->next->node->children->next->node,
                    e)
            ) || ncache__gen_name(vcache, node->children->node, e);
        default:
            assert (0);
            return -1;
        }

        break;
    case CLERI_GID_STATEMENTS:
        return ncache__list(
                syntax,
                vcache,
                node->children,
                e);
    case CLERI_GID_INDEX:
        for (   cleri_children_t * child = node->children;
                child;
                child = child->next)
            if (ncache__index(syntax, vcache, child->node, e))
                return e->nr;
        return e->nr;
    case CLERI_GID_O_NOT:
    case CLERI_GID_COMMENT:
        assert (0);
        return e->nr;
    case CLERI_GID_IMMUTABLE:
        return ncache__gen_immutable(syntax, vcache, node, e);
    case CLERI_GID_OPERATIONS:
        if (ncache__opr(syntax, vcache, node->children->next->node, e))
            return e->nr;

        if (node->children->next->next->next)     /* optional ? (sequence) */
        {
            cleri_children_t * child = \
                    node->children->next->next->next->node->children->next;

            if (ncache__i(syntax, vcache, child->node, e))
                return e->nr;

            if (child->next)        /* else : case */
                (void) ncache__i(
                        syntax,
                        vcache,
                        child->next->node->children->next->node,
                        e);
        }
        return e->nr;
    }

    assert (0);
    return e->nr;
}
