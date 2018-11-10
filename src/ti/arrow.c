/*
 * ti/arrow.h
 */
#include <assert.h>
#include <ti/arrow.h>
#include <ti.h>
#include <util/logger.h>


int ti_arrow_to_packer(cleri_node_t * arrow, qp_packer_t * packer)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (arrow->str == arrow->data)
        return qp_add_raw(packer, (const uchar * ) arrow->str, arrow->len);

    buf = malloc(arrow->len);
    if (!buf)
        return -1;

    arrow__to_buf(buf, &n);
    rc = qp_add_raw(packer, buf, n);
    free(buf);

    return rc;
}


void arrow__to_buf(cleri_node_t * nd, uchar * buf, size_t * n)
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
        }
        if (child->node->children)
            arrow__to_buf(child->node, buf, n);
    }
}
