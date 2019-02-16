/*
 * ti/arrow.h
 */
#include <assert.h>
#include <ti/arrow.h>
#include <util/logger.h>
#include <langdef/langdef.h>

static void arrow__to_buf(cleri_node_t * nd, uchar * buf, size_t * n);

cleri_node_t * ti_arrow_from_strn(const char * str, size_t n)
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
    ++node->ref;

    cleri_parse_free(res);

    return node;

fail:
    if (res)
        cleri_parse_free(res);
    free(query);
    return NULL;
}

void ti_arrow_destroy(cleri_node_t * arrow)
{
    if (!arrow || --arrow->ref)
        return;

    if (arrow->str == arrow->data)
        free(arrow->data);
    /*
     * We need to restore one reference since cleri__node_free will remove
     * one
     */
    ++arrow->ref;
    cleri__node_free(arrow);
}

int ti_arrow_to_packer(ti_arrow_t * arrow, qp_packer_t ** packer)
{
    uchar * buf;
    cleri_node_t * node;
    size_t n = 0;
    int rc;
    if (node->str == node->data)
    {
        return -(
            qp_add_map(packer) ||
            qp_add_raw(*packer, (const uchar * ) "$", 1) ||
            qp_add_raw(*packer, (const uchar * ) node->str, node->len) ||
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

int ti_arrow_to_file(cleri_node_t * arrow, FILE * f)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (arrow->str == arrow->data)
    {
        return -(
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar * ) "$", 1) ||
            qp_fadd_raw(f, (const uchar * ) arrow->str, arrow->len)
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

uchar * ti_arrow_uchar(cleri_node_t * arrow, size_t * n)
{
    uchar * buf;
    buf = malloc(arrow->len);
    if (!buf)
        return NULL;

    arrow__to_buf(arrow, buf, n);
    return buf;
}


static void arrow__to_buf(cleri_node_t * nd, uchar * buf, size_t * n)
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
        arrow__to_buf(child->node, buf, n);
}
