/*
 * ti/template.c
 */
#include <ti/val.h>
#include <ti/template.h>
#include <cleri/node.inline.h>

typedef struct
{
    size_t n;
    char s[];
} str_t;

size_t template_escaped(const char * s, size_t n)
{
    size_t count = 0;

    assert (n);
    for(; --n; ++s)
    {
        if (*s == '`' || *s == '{' || *s == '}')
        {
            ++count;
            ++s;
            if (!--n)
                break;
        }
    }
    return count;
}

str_t * template_str(const char * s, size_t n, size_t nn)
{
    char * dest;
    str_t * nstr = malloc(sizeof(str_t) + nn);
    if (!nstr)
        return NULL;

    dest = nstr->s;
    nstr->n = nn;

    for(; n--; ++s, ++dest)
    {
        *dest = *s;
        if (*s == '`' || *s == '{' || *s == '}')
        {
            ++s;
            if (!--n)
                break;
        }
    }
    return nstr;
}

int ti_template_build(cleri_node_t * node)
{
    ti_template_t * template = malloc(sizeof(ti_template_t));
    cleri_children_t * child;
    size_t count, diff;
    const char * node_end;

    if (!template)
        return -1;

    template->tp = TI_VAL_TEMPLATE;
    template->ref = 1;
    template->node = node;

    template->node->ref++;              /* take a reference */
    node_end = node->str + 1;           /* take node start (`) + 1 */

    child = node                        /* sequence */
            ->children->next->node      /* repeat */
            ->children;
    for (; child; child = child->next)
    {
        cleri_node_t * nd = child->node;
        if (nd->cl_obj->tp == CLERI_TP_REGEX)
        {
            count = template_escaped(nd->str, nd->len);

            diff = nd->str - node_end;
            nd->len += diff;
            nd->str -= diff;

            if (count)
            {
                nd->data = template_str(node_end, nd->len, nd->len - count);
                if (!nd->data)
                {
                    ti_template_destroy(template);
                    return -1;
                }
            }
        }
        node_end = nd->str + nd->len;
    }

    node->data = template;
    return 0;
}

int ti_template_compile(ti_template_t * template, ti_query_t * query, ex_t * e)
{
    cleri_node_t * node = template->node;
    cleri_children_t * child;
    size_t end_ws, n = 0;
    ti_raw_t * raw;
    unsigned char * ptr;
    const char * node_end;

    assert (query->rval == NULL);

    node_end = node->str + 1;
    child = node                        /* sequence */
            ->children->next->node      /* repeat */
            ->children;

    for (; child; child = child->next)
    {
        cleri_node_t * nd = child->node;
        if (nd->cl_obj->tp == CLERI_TP_REGEX)
        {
            n += nd->data ? ((str_t *) nd->data)->n : nd->len;
        }
        else
        {
            n += nd->str - node_end;  /* optional white space */

            if (ti_do_statement(query, nd->children->next->node, e) || (
                !ti_val_is_str(query->rval) &&
                ti_val_convert_to_str(&query->rval, e)))
                goto failed;

            n += ((ti_raw_t *) query->rval)->n;
            nd->data = query->rval;
            query->rval = NULL;
        }
        node_end = nd->str + nd->len;
    }

    end_ws = (node->str + node->len - 1) - node_end;
    n += end_ws;

    raw = malloc(sizeof(ti_raw_t) + n);
    if (!raw)
    {
        ex_set_mem(e);
        goto failed;
    }

    raw->ref = 1;
    raw->tp = TI_VAL_STR;
    raw->n = n;
    ptr = raw->data;
    child = node                        /* sequence */
            ->children->next->node      /* repeat */
            ->children;
    node_end = node->str + 1;           /* reset node end */

    for (; child; child = child->next)
    {
        cleri_node_t * nd = child->node;
        if (nd->cl_obj->tp == CLERI_TP_REGEX)
        {
            /* copy literal part */
            if (nd->data)
            {
                /* removed escaping copy */
                str_t * str = nd->data;
                memcpy(ptr, str->s, str->n);
                ptr += str->n;
            }
            else
            {
                /* original */
                memcpy(ptr, nd->str, nd->len);
                ptr += nd->len;
            }
        }
        else
        {
            ti_raw_t * expr;

            /* copy optional white space */
            n = nd->str - node_end;
            memcpy(ptr, node_end, n);
            ptr += n;

            /* copy expression value (as string) */
            expr = nd->data;
            nd->data = NULL;
            memcpy(ptr, expr->data, expr->n);
            ptr += expr->n;
            ti_val_drop((ti_val_t *) expr);
        }
        node_end = nd->str + nd->len;
    }

    /* copy optional white space at the end */
    memcpy(ptr, node_end, end_ws);

    query->rval = (ti_val_t *) raw;
    return e->nr;

failed:
    child = template->node              /* sequence */
            ->children->next->node      /* repeat */
            ->children;

    for (; child; child = child->next)
    {
        if (child->node->cl_obj->tp == CLERI_TP_SEQUENCE)
        {
            cleri_node_t * nd = child->node;
            ti_val_drop(nd->data);
            nd->data = NULL;
        }
    }
    return e->nr;
}

void ti_template_destroy(ti_template_t * template)
{
    cleri_children_t * child;

    child = template->node              /* sequence */
            ->children->next->node      /* repeat */
            ->children;

    for (; child; child = child->next)
    {
        cleri_node_t * nd = child->node;

        if (nd->cl_obj->tp == CLERI_TP_REGEX)
            free(nd->data);
        else
            ti_val_drop(nd->data);
    }

    cleri__node_free(template->node);
    free(template);
}
