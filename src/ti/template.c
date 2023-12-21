/*
 * ti/template.c
 */
#include <cleri/node.h>
#include <ti/do.h>
#include <ti/template.h>
#include <ti/val.h>
#include <ti/val.inline.h>

typedef struct
{
    size_t n;
    char s[];
} str_t;

static cleri_t regex_t = {
    .tp = CLERI_TP_REGEX,
};

size_t template_escaped(const char * s, size_t n)
{
    size_t count = 0;

    assert(n);
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

int template_child(cleri_node_t ** childaddr, const char * str, size_t n)
{
    cleri_node_t * inode = cleri__node_new(&regex_t, str, n);
    if (!inode)
        return -1;

    inode->data = NULL;
    inode->next = *childaddr;
    *childaddr = inode;
    return 0;
}

int ti_template_build(cleri_node_t * node)
{
    ti_template_t * tmplate = malloc(sizeof(ti_template_t));
    cleri_node_t ** childaddr;
    size_t count, diff = 0;
    const char * node_end;

    if (!tmplate)
        return -1;

    tmplate->tp = TI_VAL_TEMPLATE;
    tmplate->ref = 1;
    tmplate->node = node;

    tmplate->node->ref++;               /* take a reference */
    node_end = node->str + 1;           /* take node start (`) + 1 */

    childaddr = &node                   /* sequence */
            ->children->next            /* repeat */
            ->children;

    for (; *childaddr; childaddr = &(*childaddr)->next)
    {
        cleri_node_t * nd = (*childaddr);

        diff = nd->str - node_end;

        if (nd->cl_obj->tp == CLERI_TP_REGEX)
        {
            count = template_escaped(nd->str, nd->len);

            /* this will insert optional white space to this node */
            nd->len += diff;
            nd->str -= diff;

            if (count)
            {
                nd->data = template_str(node_end, nd->len, nd->len - count);
                if (!nd->data)
                    goto failed;
            }
        }
        else if (diff)
        {
            /* non-captured white space is found, add an additional node */
            if (template_child(childaddr, node_end, diff))
                goto failed;
            else
                childaddr = &(*childaddr)->next;
        }
        node_end = nd->str + nd->len;
    }

    /* check for non-captured white space at the end */
    diff = (node->str + node->len - 1) - node_end;
    if (diff && template_child(childaddr, node_end, diff))
        goto failed;

    node->data = tmplate;
    return 0;

failed:
    ti_template_destroy(tmplate);
    return -1;
}

int ti_template_compile(ti_template_t * tmplate, ti_query_t * query, ex_t * e)
{
    cleri_node_t * node = tmplate->node;
    cleri_node_t * child;
    size_t n = 0;
    ti_raw_t * raw;
    unsigned char * ptr;

    assert(query->rval == NULL);

    /*
     * First calculate the exact size, and therefore run each expression and
     * convert the result to string if required.
     */
    child = node                        /* sequence */
            ->children->next            /* repeat */
            ->children;

    for (; child; child = child->next)
    {
        if (child->cl_obj->tp == CLERI_TP_REGEX)
        {
            n += child->data ? ((str_t *) child->data)->n : child->len;
            continue;
        }

        if (ti_do_statement(query, child->children->next, e) ||
            ti_val(query->rval)->to_str(&query->rval, e))
            goto failed;

        n += ((ti_raw_t *) query->rval)->n;
        /*
         * Store the generated value to node -> cache so we can later re-use
         * the value to build the actual string. Make sure to clean those
         * both when successful and on failure.
         */
        child->data = query->rval;
        query->rval = NULL;
    }

    /* Now the exact size is known so we can allocate memory for the string */
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
            ->children->next            /* repeat */
            ->children;

    for (; child; child = child->next)
    {
        if (child->cl_obj->tp == CLERI_TP_REGEX)
        {
            /* Copy literal part */
            if (child->data)
            {
                /* Removed escaping copy */
                str_t * str = child->data;
                memcpy(ptr, str->s, str->n);
                ptr += str->n;
            }
            else
            {
                /* Original */
                memcpy(ptr, child->str, child->len);
                ptr += child->len;
            }
        }
        else
        {
            /* Copy expression value (as string) */
            ti_raw_t * expr = child->data;
            child->data = NULL;
            memcpy(ptr, expr->data, expr->n);
            ptr += expr->n;
            ti_val_unsafe_drop((ti_val_t *) expr);
        }
    }

    query->rval = (ti_val_t *) raw;
    return e->nr;

failed:
    /*
     * There might have been temporary string made and stored on a node
     * which must be destroyed. Make sure to free this memory.
     */
    child = tmplate->node               /* sequence */
            ->children->next            /* repeat */
            ->children;

    for (; child; child = child->next)
    {
        if (child->cl_obj->tp == CLERI_TP_SEQUENCE)
        {
            ti_val_drop(child->data);
            child->data = NULL;
        }
    }
    return e->nr;
}

void ti_template_destroy(ti_template_t * tmplate)
{
    cleri_node_t * child;

    child = tmplate->node               /* sequence */
            ->children->next            /* repeat */
            ->children;

    for (; child; child = child->next)
    {
        if (child->cl_obj->tp == CLERI_TP_REGEX)
            free(child->data);
        else
            ti_val_drop(child->data);
    }

    cleri__node_free(tmplate->node);
    free(tmplate);
}
