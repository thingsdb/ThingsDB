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
    ncache->immutable_cache = vec_new(n);

    if (!ncache->immutable_cache)
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
    vec_destroy(ncache->immutable_cache, (vec_destroy_cb) ti_val_unsafe_drop);
    free(ncache->query);
    free(ncache);
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
        cleri_node_t * child,
        ex_t * e)
{
    for (; child; child = child->next->next)
        if (ncache__statement(syntax, vcache, child, e) || !child->next)
            break;
    return e->nr;
}

static int ncache__index(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    cleri_node_t * child = nd->children;
    assert (nd->cl_obj->gid == CLERI_GID_INDEX);
    assert (child);
    do
    {
        cleri_node_t * c = child            /* sequence */
                ->children->next            /* slice */
                ->children;

        if (child->children->next->next->next &&
            ncache__statement(
                    syntax,
                    vcache,
                    child                             /* sequence */
                    ->children->next->next->next      /* assignment */
                    ->children->next, e))             /* statement */
            return e->nr;

        for (; c; c = c->next)
            if (c->cl_obj->gid == CLERI_GID_STATEMENT &&
                ncache__statement(syntax, vcache, c, e))
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
    nd = nd                         /* sequence */
            ->children->next        /* list */
            ->children;
    for (; nd; nd = nd->next ? nd->next->next : NULL)
    {
        /* sequence(name: statement) (only investigate the statements */
        if (ncache__statement(
                syntax,
                vcache,
                nd->children->next->next,
                e))
            return e->nr;
    }
    return e->nr;
}

static int ncache__closure(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    void ** data = &nd->children->data;

    if (ncache__statement(
                syntax,
                vcache,
                nd->children->next->next->next,
                e))
        return e->nr;

    *data = ti_closure_from_node(
            nd,
            (syntax->flags & TI_QBIND_FLAG_THINGSDB)
                        ? TI_CLOSURE_FLAG_BTSCOPE
                        : TI_CLOSURE_FLAG_BCSCOPE);

    assert (vec_space(vcache));

    if (*data)
        VEC_push(vcache, *data);
    else
        ex_set_mem(e);

    return e->nr;
}

static inline int ncache__enum(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    return (nd = nd->children->next)->cl_obj->gid == CLERI_GID_CLOSURE
        ? ncache__closure(syntax, vcache, nd, e)
        : 0;
}

static int ncache__varname_opt_fa(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    if (!nd->children->next)
        return ncache__gen_name(vcache, nd->children, e);

    switch (nd->children->next->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        return (ncache__list(
                syntax,
                vcache,
                nd->children->next->children->next->children,
                e)
        ) || (
                !nd->data &&
                ncache__gen_name(vcache, nd->children, e)
        ) ? e->nr : 0;
    case CLERI_GID_ASSIGN:
        return (ncache__statement(
                syntax,
                vcache,
                nd->children->next->children->next,
                e)
        ) || ncache__gen_name(vcache, nd->children, e) ? e->nr : 0;
    case CLERI_GID_INSTANCE:
        return ncache__thing(syntax, vcache, nd->children->next, e);
    case CLERI_GID_ENUM_:
        return ncache__enum(syntax, vcache, nd->children->next, e);
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

    if (ncache__varname_opt_fa(syntax, vcache, nd->children->next, e))
        return e->nr;

    /* index */
    if (nd->children->next->next->children &&
        ncache__index(syntax, vcache, nd->children->next->next, e))
        return e->nr;

    /* chain */
    if (nd->children->next->next->next)
        (void) ncache__chain(
                syntax,
                vcache,
                nd->children->next->next->next,
                e);
    return e->nr;
}

static int ncache__gen_template(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    for(cleri_node_t * child = nd       /* sequence */
            ->children->next            /* repeat */
            ->children;
        child;
        child = child->next)
        if (child->cl_obj->tp == CLERI_TP_SEQUENCE &&
            ncache__statement(
                    syntax,
                    vcache,
                    child->children->next,
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
        cleri_node_t * child = nd           /* sequence */
                ->children->next            /* list */
                ->children;
        for (; child; child = child->next ? child->next->next : NULL)
        {
            /* sequence(name: statement) (only investigate the statements */
            if (ncache__statement(
                    syntax,
                    vcache,
                    child->children->next->next,
                    e))
                return e->nr;
        }
        return e->nr;
    }
    case CLERI_GID_ARRAY:
        return ncache__list(
                syntax,
                vcache,
                nd->children->next->children,
                e);
    case CLERI_GID_BLOCK:
        return ncache__list(
                syntax,
                vcache,
                nd->children->next->next->children,
                e);
    case CLERI_GID_PARENTHESIS:
        return ncache__statement(syntax, vcache, nd->children->next, e);
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
    if (ncache__expr_choice(syntax, vcache, nd->children->next, e))
        return e->nr;

    /* index */
    if (nd->children->next->next->children &&
        ncache__index(syntax, vcache, nd->children->next->next, e))
        return e->nr;

    /* chain */
    if (nd->children->next->next->next)
        (void) ncache__chain(
                syntax,
                vcache,
                nd->children->next->next->next,
                e);
    return e->nr;
}

static int ncache__operations(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    uint32_t gid = nd->children->next->cl_obj->gid;
    if (gid == CLERI_GID_OPR8_TERNARY &&
        ncache__statement(
                syntax,
                vcache,
                nd->children->next->children->next,
                e))
        return e->nr;

    return (
        ncache__statement(syntax, vcache, nd->children, e) ||
        ncache__statement(syntax, vcache, nd->children->next->next, e)
    ) ? e->nr : 0;
}

static int ncache__if_statement(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_IF_STATEMENT);

    if (ncache__statement(syntax, vcache, nd->children->next->next, e) ||
        ncache__statement(syntax, vcache, nd->children->data, e))
        return e->nr;

    return nd->children->next->data
        ? ncache__statement(syntax, vcache, nd->children->next->data, e)
        : 0;
}

static int ncache__return_statement(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    if (ncache__statement(syntax, vcache, nd->children->next, e))
        return -1;

    return nd->children->data
        ? ncache__statement(syntax, vcache, nd->children->data, e)
        : 0;
}

static int ncache__for_statement(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    cleri_node_t * tmp;

    nd = nd->
            children->              /* for  */
            next->                  /* (    */
            next;                   /* List(variable) */

    for(tmp = nd->children; tmp; tmp = tmp->next ? tmp->next->next : NULL)
        if (ncache__gen_name(vcache, tmp, e))
            return e->nr;

    ncache__statement(syntax, vcache, (nd = nd->next->next), e);
    ncache__statement(syntax, vcache, (nd = nd->next->next), e);
    return e->nr;
}

static int ncache__statement(
        ti_qbind_t * syntax,
        vec_t * vcache,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT);
    switch ((nd = nd->children)->cl_obj->gid)
    {
        case CLERI_GID_IF_STATEMENT:
            return ncache__if_statement(syntax, vcache, nd, e);
        case CLERI_GID_RETURN_STATEMENT:
            return ncache__return_statement(syntax, vcache, nd, e);
        case CLERI_GID_FOR_STATEMENT:
            return ncache__for_statement(syntax, vcache, nd, e);
        case CLERI_GID_K_CONTINUE:
        case CLERI_GID_K_BREAK:
            return 0;
        case CLERI_GID_CLOSURE:
            return ncache__closure(syntax, vcache, nd, e);
        case CLERI_GID_EXPRESSION:
            return ncache__expression(syntax, vcache, nd, e);
        case CLERI_GID_OPERATIONS:
            return ncache__operations(syntax, vcache, nd, e);
    }
    assert (0);
    return -1;
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
        for (nd = nd->children; nd; nd = nd->next->next)
            if (ncache__statement(syntax, vcache, nd, e) || !nd->next)
                return e->nr;
        return e->nr;
    }
    return ncache__statement(syntax, vcache, nd, e);
}


