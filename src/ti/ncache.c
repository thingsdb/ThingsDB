/*
 * ti/ncache.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <ti/closure.h>
#include <ti/names.h>
#include <ti/ncache.h>
#include <ti/raw.h>
#include <ti/regex.h>
#include <ti/template.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/logger.h>
#include <util/strx.h>

static int ncache__statement(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e);

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
    vec_destroy(ncache->val_cache, (vec_destroy_cb) ti_val_unsafe_drop);
    free(ncache->query);
    free(ncache);
}

int ncache__gen_immutable(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_T_CLOSURE:
        if (ncache__statement(
                    syntax,
                    vcache,
                    nd->children->next->next->next->node,
                    e))
            return e->nr;
        nd->data = ti_closure_from_node(
                nd,
                (syntax->flags & TI_QBIND_FLAG_THINGSDB)
                            ? TI_VFLAG_CLOSURE_BTSCOPE
                            : TI_VFLAG_CLOSURE_BCSCOPE);
        break;
    case CLERI_GID_T_FLOAT:
        nd->data = ti_vfloat_create(strx_to_double(nd->str, NULL));
        break;
    case CLERI_GID_T_INT:
    {
        int64_t i = strx_to_int64(nd->str, NULL);
        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }
        nd->data = ti_vint_create(i);
        break;
    }
    case CLERI_GID_T_REGEX:
        nd->data = ti_regex_from_strn(nd->str, nd->len, e);
        if (!nd->data)
            return e->nr;
        break;
    case CLERI_GID_T_STRING:
        nd->data = ti_str_from_ti_string(nd->str, nd->len);
        break;
    default:
        return 0;
    }

    assert (vec_space(vcache));

    if (nd->data)
        VEC_push(vcache, nd->data);
    else
        ex_set_mem(e);

    return e->nr;
}

int ncache__gen_name(vec_t * vcache, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_NAME ||
            nd->cl_obj->gid == CLERI_GID_VAR);
    assert (vec_space(vcache));

    nd->data = ti_names_get(nd->str, nd->len);

    if (nd->data)
        VEC_push(vcache, nd->data);
    else
        ex_set_mem(e);
    return e->nr;
}

static inline int ncache__list(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_children_t * child,
        ex_t * e)
{
    for (; child; child = child->next->next)
        if (ncache__statement(syntax, vcache, child->node, e) || !child->next)
            break;
    return e->nr;
}

static int ncache__index(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    cleri_children_t * child = nd->children;
    assert (nd->cl_obj->gid == CLERI_GID_INDEX);
    assert (child);
    do
    {
        cleri_children_t * c = child->node      /* sequence */
                ->children->next->node              /* slice */
                ->children;

        if (child->node->children->next->next->next &&
            ncache__statement(
                    syntax,
                    vcache,
                    child->node                             /* sequence */
                    ->children->next->next->next->node      /* assignment */
                    ->children->next->node, e))             /* statement */
            return e->nr;

        for (; c; c = c->next)
            if (c->node->cl_obj->gid == CLERI_GID_STATEMENT &&
                ncache__statement(syntax, vcache, c->node, e))
                return e->nr;
    }
    while((child = child->next));

    return e->nr;
}

static inline int ncache__thing(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    cleri_children_t * child = nd           /* sequence */
            ->children->next->node          /* list */
            ->children;
    for (; child; child = child->next ? child->next->next : NULL)
    {
        /* sequence(name: statement) (only investigate the statements */
        if (ncache__statement(
                syntax,
                vcache,
                child->node->children->next->next->node,
                e))
            return e->nr;
    }
    return e->nr;
}

static int ncache__enum(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    nd->data = NULL;  /* member value */

    nd = nd->children->next->node;
    switch(nd->cl_obj->gid)
    {
    case CLERI_GID_NAME:
        nd->data = vcache;  /* trick so we can assign a enum value to the
                               correct cache */
        break;
    case CLERI_GID_T_CLOSURE:
        if (ncache__statement(
                    syntax,
                    vcache,
                    nd->children->next->next->next->node,
                    e))
            return e->nr;
        nd->data = ti_closure_from_node(
                nd,
                (syntax->flags & TI_QBIND_FLAG_THINGSDB)
                            ? TI_VFLAG_CLOSURE_BTSCOPE
                            : TI_VFLAG_CLOSURE_BCSCOPE);
        if (nd->data)
            VEC_push(vcache, nd->data);
        else
            ex_set_mem(e);
    }
    return e->nr;
}

static int ncache__varname_opt_fa(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    if (!nd->children->next)
        return ncache__gen_name(vcache, nd->children->node, e);

    switch (nd->children->next->node->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        return (ncache__list(
                syntax,
                vcache,
                nd->children->next->node->children->next->node->children,
                e)
        ) || (
                !nd->data &&
                ncache__gen_name(vcache, nd->children->node, e)
        ) ? e->nr : 0;
    case CLERI_GID_ASSIGN:
        return (ncache__statement(
                syntax,
                vcache,
                nd->children->next->node->children->next->node,
                e)
        ) || ncache__gen_name(vcache, nd->children->node, e) ? e->nr : 0;
    case CLERI_GID_INSTANCE:
        return ncache__thing(syntax, vcache, nd->children->next->node, e);
    case CLERI_GID_ENUM_:
        return ncache__enum(syntax, vcache, nd->children->next->node, e);
    }

    assert (0);
    return -1;
}

static int ncache__chain(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);

    if (ncache__varname_opt_fa(syntax, vcache, nd->children->next->node, e))
        return e->nr;

    /* index */
    if (nd->children->next->next->node->children &&
        ncache__index(syntax, vcache, nd->children->next->next->node, e))
        return e->nr;

    /* chain */
    if (nd->children->next->next->next)
        (void) ncache__chain(
                syntax,
                vcache,
                nd->children->next->next->next->node,
                e);
    return e->nr;
}

static int ncache__gen_template(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    for(cleri_children_t * child = nd       /* sequence */
            ->children->next->node          /* repeat */
            ->children;
        child;
        child = child->next)
        if (child->node->cl_obj->tp == CLERI_TP_SEQUENCE &&
            ncache__statement(
                    syntax,
                    vcache,
                    child->node->children->next->node,
                    e))
            return e->nr;

    if (ti_template_build(nd) == 0)
    {
        assert (vec_space(vcache));
        VEC_push(vcache, nd->data);
    }
    else
        ex_set_mem(e);

    return e->nr;
}

static int ncache__expr_choice(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        return ncache__chain(syntax, vcache, nd, e);
    case CLERI_GID_THING_BY_ID:
        nd->data = ti_vint_create(strtoll(nd->str + 1, NULL, 10));
        if (!nd->data)
            ex_set_mem(e);
        return e->nr;
    case CLERI_GID_T_CLOSURE:
        if (ncache__statement(
                    syntax,
                    vcache,
                    nd->children->next->next->next->node,
                    e))
            return e->nr;
        nd->data = ti_closure_from_node(
                nd,
                (syntax->flags & TI_QBIND_FLAG_THINGSDB)
                            ? TI_VFLAG_CLOSURE_BTSCOPE
                            : TI_VFLAG_CLOSURE_BCSCOPE);
        break;
    case CLERI_GID_T_FLOAT:
        nd->data = ti_vfloat_create(strx_to_double(nd->str, NULL));
        break;
    case CLERI_GID_T_INT:
    {
        int64_t i = strx_to_int64(nd->str, NULL);
        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }
        nd->data = ti_vint_create(i);
        break;
    }
    case CLERI_GID_T_REGEX:
        nd->data = ti_regex_from_strn(nd->str, nd->len, e);
        if (!nd->data)
            return e->nr;
        break;
    case CLERI_GID_T_STRING:
        nd->data = ti_str_from_ti_string(nd->str, nd->len);
        break;
    case CLERI_GID_TEMPLATE:
        return ncache__gen_template(syntax, vcache, nd, e);
    case CLERI_GID_VAR_OPT_MORE:
        return ncache__varname_opt_fa(syntax, vcache, nd, e);
    case CLERI_GID_THING:
    {
        cleri_children_t * child = nd           /* sequence */
                ->children->next->node          /* list */
                ->children;
        for (; child; child = child->next ? child->next->next : NULL)
        {
            /* sequence(name: statement) (only investigate the statements */
            if (ncache__statement(
                    syntax,
                    vcache,
                    child->node->children->next->next->node,
                    e))
                return e->nr;
        }
        return e->nr;
    }
    case CLERI_GID_ARRAY:
        return ncache__list(
                syntax,
                vcache,
                nd->children->next->node->children,
                e);
    case CLERI_GID_BLOCK:
        return ncache__list(
                syntax,
                vcache,
                nd->children->next->next->node->children,
                e);
    case CLERI_GID_PARENTHESIS:
        return ncache__statement(syntax, vcache, nd->children->next->node, e);
    default:
        return e->nr;
    }

    /* immutable values should `break` in the switch statement above */
    assert (vec_space(vcache));

    if (nd->data)
        VEC_push(vcache, nd->data);
    else
        ex_set_mem(e);

    return e->nr;
}

static inline int ncache__expression(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_EXPRESSION);

    if (ncache__expr_choice(syntax, vcache, nd->children->next->node, e))
        return e->nr;

    /* index */
    if (nd->children->next->next->node->children &&
        ncache__index(syntax, vcache, nd->children->next->next->node, e))
        return e->nr;

    /* chain */
    if (nd->children->next->next->next)
        (void) ncache__chain(
                syntax,
                vcache,
                nd->children->next->next->next->node,
                e);
    return e->nr;
}

static int ncache__operations(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    uint32_t gid = nd->children->next->node->cl_obj->gid;
    if (gid == CLERI_GID_OPR8_TERNARY &&
        ncache__statement(
                syntax,
                vcache,
                nd->children->next->node->children->next->node,
                e))
        return e->nr;

    return (
        ncache__statement(syntax, vcache, nd->children->node, e) ||
        ncache__statement(syntax, vcache, nd->children->next->next->node, e)
    ) ? e->nr : 0;
}

static int ncache__statement(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT);
    return (nd = nd->children->node)->cl_obj->gid == CLERI_GID_EXPRESSION
            ? ncache__expression(syntax, vcache, nd, e)
            : ncache__operations(syntax, vcache, nd, e);
}

int ti_ncache_gen_node_data(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT ||
            nd->cl_obj->gid == CLERI_GID_STATEMENTS);

    if (nd->cl_obj->gid == CLERI_GID_STATEMENTS)
    {
        cleri_children_t * child = nd->children;

        for (; child; child = child->next->next)
        {
            if (ncache__statement(syntax, vcache, child->node, e) ||
                !child->next)
                return e->nr;
        }
        return e->nr;
    }
    return ncache__statement(syntax, vcache, nd, e);
}


