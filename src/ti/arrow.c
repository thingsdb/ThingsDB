/*
 * ti/arrow.h
 */
#include <assert.h>
#include <ti/arrow.h>
#include <util/logger.h>

static void arrow__to_buf(cleri_node_t * nd, uchar * buf, size_t * n);

int ti_arrow_to_packer(cleri_node_t * arrow, qp_packer_t * packer)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (arrow->str == arrow->data)
        return qp_add_raw(packer, (const uchar * ) arrow->str, arrow->len);

    buf = ti_arrow_uchar(arrow, &n);
    if (!buf)
        return -1;

    rc = qp_add_raw(packer, buf, n);
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
        return (
            qp_fadd_type(f, QP_HOOK) ||
            qp_fadd_raw(f, (const uchar * ) arrow->str, arrow->len)
        );
    }
    buf = ti_arrow_uchar(arrow, &n);
    if (!buf)
        return -1;
    rc = qp_fadd_type(f, QP_HOOK) || qp_fadd_raw(f, buf, n);
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
    cleri_children_t * child;

    for (child = nd->children; child; child = child->next)
    {
        switch (child->node->cl_obj->tp)
        {
        case CLERI_TP_KEYWORD:
        case CLERI_TP_TOKEN:
        case CLERI_TP_TOKENS:
        case CLERI_TP_REGEX:
            memcpy(buf + *n, nd->str, nd->len);
            *n += nd->len;
            continue;
        default:
            break;
        }
        if (child->node->children)
            arrow__to_buf(child->node, buf, n);
    }
}
