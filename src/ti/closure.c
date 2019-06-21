/*
 * ti/closure.h
 */
#include <assert.h>
#include <util/logger.h>
#include <langdef/langdef.h>
#include <ti/closure.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/regex.h>
#include <util/strx.h>

/*
 * Assigned closures require to own a `query` string, and they need to prepare
 * all the `primitives` cached values since a query which is using the closure
 * does not have space to store these. (and it improves the performance of
 * the closure)
 */
typedef struct closure__ubound_s
{
    char * query;
    vec_t * nd_val_cache;
} closure__ubound_t;

static closure__ubound_t * closure__ubound_create(char * query, size_t n)
{
    closure__ubound_t * ubound = malloc(sizeof(closure__ubound_t));
    if (!ubound)
        return NULL;

    ubound->query = query;
    ubound->nd_val_cache = vec_new(n);

    if (!ubound->nd_val_cache)
    {
        free(ubound);
        return NULL;
    }

    return ubound;
}

static void closure__ubound_destroy(closure__ubound_t * ubound)
{
    if (!ubound)
        return;
    vec_destroy(ubound->nd_val_cache, (vec_destroy_cb) ti_val_drop);
    free(ubound->query);
    free(ubound);
}

static int closure__gen_primitives(vec_t * cache, cleri_node_t * nd, ex_t * e)
{
    if (nd->cl_obj->gid == CLERI_GID_PRIMITIVES)
    {
        nd = nd->children->node;
        switch (nd->cl_obj->gid)
        {
        case CLERI_GID_T_FLOAT:
            assert (!nd->data);
            nd->data = ti_vfloat_create(strx_to_double(nd->str));
            break;
        case CLERI_GID_T_INT:
            assert (!nd->data);
            nd->data = ti_vint_create(strx_to_int64(nd->str));
            if (errno == ERANGE)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            break;
        case CLERI_GID_T_REGEX:
            assert (!nd->data);
            nd->data = ti_regex_from_strn(nd->str, nd->len, e);
            if (!nd->data)
                return e->nr;
            break;
        case CLERI_GID_T_STRING:
            assert (!nd->data);
            nd->data = ti_raw_from_ti_string(nd->str, nd->len);
            break;

        default:
            return 0;
        }
        assert (vec_space(cache));

        if (nd->data)
            VEC_push(cache, nd->data);
        else
            ex_set_alloc(e);

        return e->nr;
    }

    for (cleri_children_t * child = nd->children; child; child = child->next)
        if (closure__gen_primitives(cache, child->node, e))
            return e->nr;

    return e->nr;
}

static cleri_node_t * closure__node_from_strn(
        const char * str,
        size_t n,
        ex_t * e)
{
    closure__ubound_t * ubound;
    ti_syntax_t syntax;
    cleri_parse_t * res;
    cleri_node_t * node;
    char * query = strndup(str, n);
    if (!query)
    {
        ex_set_alloc(e);
        return NULL;
    }

    ti_syntax_init(&syntax);

    res = cleri_parse2(
            ti()->langdef,
            query,
            CLERI_FLAG_EXPECTING_DISABLED); /* only error position */
    if (!res)
    {
        ex_set_alloc(e);
        goto fail0;
    }

    if (!res->is_valid)
    {
        ex_set(e, EX_SYNTAX_ERROR, "invalid syntax in closure");
        goto fail1;
    }

    node = res->tree->children->node        /* Sequence (START) */
            ->children->next->node;         /* List of statements */

    /* we should have exactly one statement */
    if (!node->children || node->children->next)
    {
        ex_set(e, EX_BAD_DATA, "closure is expecting exactly one node");
        goto fail1;
    }

    node = node                             /* List of statements */
            ->children->node                /* Sequence - scope */
            ->children->next->node          /* Choice */
            ->children->node;               /* closure */

    if (node->cl_obj->gid != CLERI_GID_CLOSURE)
    {
        ex_set(e, EX_INDEX_ERROR, "node is not a closure");
        goto fail1;
    }

    /*  closure = Sequence('|', List(name, opt=True), '|', scope)  */
    ti_syntax_investigate(&syntax, node->children->next->next->next->node);

    ubound = closure__ubound_create(query, syntax.nd_val_cache_n);
    if (!ubound)
    {
        ex_set_alloc(e);
        goto fail1;
    }

    node->data = ubound;
    if (closure__gen_primitives(ubound->nd_val_cache, node, e))
        goto fail2;

    /* make sure the node gets an extra reference so it will be kept */
    ++node->ref;

    cleri_parse_free(res);
    return node;

fail2:
    closure__ubound_destroy(ubound);
fail1:
    cleri_parse_free(res);
fail0:
    free(query);
    return NULL;
}

static void closure__node_to_buf(cleri_node_t * nd, uchar * buf, size_t * n)
{
    switch (nd->cl_obj->tp)
    {
    case CLERI_TP_KEYWORD:
    case CLERI_TP_TOKEN:
    case CLERI_TP_TOKENS:
    case CLERI_TP_REGEX:
        memcpy(buf + (*n), nd->str, nd->len);
        (*n) += nd->len;
        return;
    case CLERI_TP_SEQUENCE:
    case CLERI_TP_OPTIONAL:
    case CLERI_TP_CHOICE:
    case CLERI_TP_LIST:
    case CLERI_TP_REPEAT:
    case CLERI_TP_PRIO:
    case CLERI_TP_RULE:
    case CLERI_TP_THIS:
    case CLERI_TP_REF:
    case CLERI_TP_END_OF_STATEMENT:
        break;
    }

    for (cleri_children_t * child = nd->children; child; child = child->next)
        closure__node_to_buf(child->node, buf, n);
}


/*
 * Return a closure which is bound to the query. The node for this closure can
 * only be used for as long as the 'query' exists in memory. If the closure
 * will be stored for later usage, a call to `ti_closure_unbound` must be
 * made.
 */
ti_closure_t * ti_closure_from_node(cleri_node_t * node)
{
    ti_closure_t * closure = malloc(sizeof(ti_closure_t));
    if (!closure)
        return NULL;

    closure->ref = 1;
    closure->tp = TI_VAL_CLOSURE;
    closure->flags = (uintptr_t) node->data;
    closure->node = node;

    return closure;
}

ti_closure_t * ti_closure_from_strn(const char * str, size_t n, ex_t * e)
{
    ti_closure_t * closure = malloc(sizeof(ti_closure_t));
    if (!closure)
        return NULL;

    closure->ref = 1;
    closure->tp = TI_VAL_CLOSURE;
    closure->flags = 0;
    closure->node = closure__node_from_strn(str, n, e);
    if (!closure->node)
    {
        free(closure);
        return NULL;
    }

    return closure;
}

void ti_closure_destroy(ti_closure_t * closure)
{
    if (!closure)
        return;

    if (~closure->flags & TI_VFLAG_CLOSURE_QBOUND)
    {
        closure__ubound_destroy((closure__ubound_t *) closure->node->data);
        cleri__node_free(closure->node);
    }

    free(closure);
}

int ti_closure_unbound(ti_closure_t * closure, ex_t * e)
{
    cleri_node_t * node;

    assert (~closure->flags & TI_VFLAG_CLOSURE_WSE);
    if (~closure->flags & TI_VFLAG_CLOSURE_QBOUND)
        return e->nr;

    node = closure__node_from_strn(closure->node->str, closure->node->len, e);
    if (!node)
        return e->nr;

    closure->node = node;
    closure->flags &= ~TI_VFLAG_CLOSURE_QBOUND;

    return e->nr;
}

int ti_closure_to_packer(ti_closure_t * closure, qp_packer_t ** packer)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (~closure->flags & TI_VFLAG_CLOSURE_QBOUND)
    {
        return -(
            qp_add_map(packer) ||
            qp_add_raw(*packer, (const uchar * ) "$", 1) ||
            qp_add_raw(
                    *packer,
                    (const uchar * ) closure->node->str,
                    closure->node->len) ||
            qp_close_map(*packer)
        );
    }
    buf = ti_closure_uchar(closure, &n);
    if (!buf)
        return -1;

    rc = -(
        qp_add_map(packer) ||
        qp_add_raw(*packer, (const uchar * ) "$", 1) ||
        qp_add_raw(*packer, buf, n) ||
        qp_close_map(*packer)
    );

    free(buf);
    return rc;
}

int ti_closure_to_file(ti_closure_t * closure, FILE * f)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (~closure->flags & TI_VFLAG_CLOSURE_QBOUND)
    {
        return -(
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar * ) "$", 1) ||
            qp_fadd_raw(f, (const uchar * ) closure->node->str, closure->node->len)
        );
    }
    buf = ti_closure_uchar(closure, &n);
    if (!buf)
        return -1;
    rc = -(
        qp_fadd_type(f, QP_MAP1) ||
        qp_fadd_raw(f, (const uchar * ) "$", 1) ||
        qp_fadd_raw(f, buf, n)
    );
    free(buf);
    return rc;
}

uchar * ti_closure_uchar(ti_closure_t * closure, size_t * n)
{
    uchar * buf;
    buf = malloc(closure->node->len);
    if (!buf)
        return NULL;

    closure__node_to_buf(closure->node, buf, n);
    return buf;
}

