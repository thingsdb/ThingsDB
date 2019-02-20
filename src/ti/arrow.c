/*
 * ti/arrow.h
 */
#include <assert.h>
#include <ti/arrow.h>
#include <util/logger.h>
#include <langdef/langdef.h>

static cleri_node_t * arrow__node_from_strn(const char * str, size_t n);
static void arrow__node_to_buf(cleri_node_t * nd, uchar * buf, size_t * n);

/*
 * Return an arrow with is bound to the query. The node for this arrow can
 * only be used for as long as the 'query' exists in memory. If the arrow
 * will be stored for later usage, a call to `ti_arrow_unbound` must be
 * made.
 */
ti_arrow_t * ti_arrow_from_node(cleri_node_t * node)
{
    ti_arrow_t * arrow = malloc(sizeof(ti_arrow_t));
    if (!arrow)
        return NULL;

    arrow->flags = (uintptr_t) node->data;
    arrow->node = node;
}

ti_arrow_t * ti_arrow_from_strn(const char * str, size_t n)
{
    ti_arrow_t * arrow = malloc(sizeof(ti_arrow_t));
    if (!arrow)
        return NULL;

    arrow->flags = 0;
    arrow->node = arrow__node_from_strn(str, n);
    if (!arrow->node)
    {
        free(arrow);
        return NULL;
    }

    return arrow;
}

void ti_arrow_destroy(ti_arrow_t * arrow)
{
    if (!arrow)
        return;

    if (~arrow->flags & TI_ARROW_FLAG_QBOUND)
    {
        free(arrow->node->data);
        cleri__node_free(arrow->node);
    }

    free(arrow);
}

int ti_arrow_unbound(ti_arrow_t * arrow)
{
    cleri_node_t * node;

    assert (~arrow->flags & TI_ARROW_FLAG_WSE);
    if (~arrow->flags & TI_ARROW_FLAG_QBOUND)
        return 0;

    node = arrow__node_from_strn(arrow->node->str, arrow->node->len);
    if (!node)
        return -1;

    arrow->node = node;
    arrow->flags &= ~TI_ARROW_FLAG_QBOUND;

    return 0;
}

int ti_arrow_to_packer(ti_arrow_t * arrow, qp_packer_t ** packer)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (~arrow->flags & TI_ARROW_FLAG_QBOUND)
    {
        return -(
            qp_add_map(packer) ||
            qp_add_raw(*packer, (const uchar * ) "$", 1) ||
            qp_add_raw(
                    *packer,
                    (const uchar * ) arrow->node->str,
                    arrow->node->len) ||
            qp_close_map(*packer)
        );
    }
    buf = ti_arrow_uchar(arrow, &n);
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

int ti_arrow_to_file(ti_arrow_t * arrow, FILE * f)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (~arrow->flags & TI_ARROW_FLAG_QBOUND)
    {
        return -(
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar * ) "$", 1) ||
            qp_fadd_raw(f, (const uchar * ) arrow->node->str, arrow->node->len)
        );
    }
    buf = ti_arrow_uchar(arrow, &n);
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

uchar * ti_arrow_uchar(ti_arrow_t * arrow, size_t * n)
{
    uchar * buf;
    buf = malloc(arrow->node->len);
    if (!buf)
        return NULL;

    arrow__node_to_buf(arrow->node, buf, n);
    return buf;
}

static cleri_node_t * arrow__node_from_strn(const char * str, size_t n)
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
            ->children->node;               /* arrow */

    if (node->cl_obj->gid != CLERI_GID_ARROW)
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

static void arrow__node_to_buf(cleri_node_t * nd, uchar * buf, size_t * n)
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
        arrow__node_to_buf(child->node, buf, n);
}
