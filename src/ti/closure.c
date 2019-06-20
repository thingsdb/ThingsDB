/*
 * ti/closure.h
 */
#include <assert.h>
#include <util/logger.h>
#include <langdef/langdef.h>
#include <ti/closure.h>

static cleri_node_t * closure__node_from_strn(const char * str, size_t n);
static void closure__node_to_buf(cleri_node_t * nd, uchar * buf, size_t * n);

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

ti_closure_t * ti_closure_from_strn(const char * str, size_t n)
{
    ti_closure_t * closure = malloc(sizeof(ti_closure_t));
    if (!closure)
        return NULL;

    closure->flags = 0;
    closure->node = closure__node_from_strn(str, n);
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
        free(closure->node->data);
        cleri__node_free(closure->node);
    }

    free(closure);
}

int ti_closure_unbound(ti_closure_t * closure)
{
    cleri_node_t * node;

    assert (~closure->flags & TI_VFLAG_CLOSURE_WSE);
    if (~closure->flags & TI_VFLAG_CLOSURE_QBOUND)
        return 0;

    node = closure__node_from_strn(closure->node->str, closure->node->len);
    if (!node)
        return -1;

    closure->node = node;
    closure->flags &= ~TI_VFLAG_CLOSURE_QBOUND;

    return 0;
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

static cleri_node_t * closure__node_from_strn(const char * str, size_t n)
{
    cleri_parse_t * res;
    cleri_node_t * node;
    char * query = strndup(str, n);
    if (!query)
        return NULL;

    res = cleri_parse2(
            ti()->langdef,
            query,
            CLERI_FLAG_EXPECTING_DISABLED); /* only error position */
    if (!res || !res->is_valid)
        goto fail;

    node = res->tree->children->node        /* Sequence (START) */
            ->children->next->node;         /* List of statements */

    /* we should have exactly one statement */
    if (!node->children || node->children->next)
        goto fail;

    node = node                             /* List of statements */
            ->children->node                /* Sequence - scope */
            ->children->next->node          /* Choice */
            ->children->node;               /* closure */

    if (node->cl_obj->gid != CLERI_GID_CLOSURE)
        goto fail;

    node->data = query;

    /* make sure the node gets an extra reference so it will be kept */
    ++node->ref;

    cleri_parse_free(res);

    return node;

fail:
    if (res)
        cleri_parse_free(res);
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
