/*
 * ti/fmt.c
 */
#include <assert.h>
#include <ti/fmt.h>
#include <langdef/langdef.h>

static int fmt__statement(ti_fmt_t * fmt, cleri_node_t * nd);

static inline int fmt__indent_str(ti_fmt_t * fmt, const char * str)
{
    int spaces = fmt->indent * fmt->indent_spaces;
    return buf_append_fmt(&fmt->buf, "\n%*s%s", spaces, "", str);
}

static inline _Bool fmt__chain_is_func(cleri_node_t * nd)
{
    cleri_children_t * child = nd->children->next->node->children->next;
    return  child && child->node->cl_obj->gid == CLERI_GID_FUNCTION;
}

static inline _Bool fmt__chain_next_is_func(cleri_node_t * nd)
{
    cleri_children_t * child = nd->children->next->next->next;
    return  child && fmt__chain_is_func(child->next->node);
}

static int fmt__index(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child = nd->children;
    do
    {
        cleri_children_t * c = child->node     /* sequence */
                ->children->next->node         /* slice */
                ->children;

        if (buf_append_str(&fmt->buf, "["))
            return -1;


        for (; c; c = c->next)
            if (c->node->cl_obj->gid == CLERI_GID_STATEMENT)
                fmt__statement(fmt, c->node);

        if (buf_append_str(&fmt->buf, "]"))
            return -1;

        if (child->node->children->next->next->next && (
            buf_append_str(&fmt->buf, " = ") ||
            fmt__statement(
                    fmt,
                    child->node
                        ->children->next->next->next->node
                        ->children->next->node)))
            return -1;
    }
    while ((child = child->next));

    return 0;
}

static int fmt__chain(ti_fmt_t * fmt, cleri_node_t * nd, _Bool with_indent)
{
    cleri_children_t * child = nd->children->next;

    if (fmt__chain_is_func(nd))
    {
        if (!with_indent && fmt__chain_next_is_func(nd))
        {
            with_indent = true;
            ++fmt->indent;
        }
    }

    if (with_indent)
    {
        if (fmt__indent_str(fmt, "."))
            return -1;
    }
    else
    {
        if (buf_append_str(&fmt->buf, "."))
            return -1;
    }

    /* index */
    if ((child = child->next)->node->children)
        if (fmt__index(fmt, child->node))
            return -1;

    /* chain */
    return child->next? fmt__chain(fmt, child->next->node, with_indent) : 0;
}

static int fmt__closure(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * first, * child;

    if (buf_append_str(&fmt->buf, "|"))
        return -1;

    first = nd                          /* sequence */
            ->children->next->node      /* list */
            ->children;                 /* first child */


    for(child = first; child; child = child->next ? child->next->next : NULL)
    {
        if (child != first)
            if (buf_append_str(&fmt->buf, ", "))
                return -1;

        if (buf_append(&fmt->buf, child->node->str, child->node->len))
            return -1;
    }

    if (buf_append_str(&fmt->buf, "| "))
        return -1;

    return 0;
}


static int fmt__expr_choice(ti_fmt_t * fmt, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        return fmt__chain(fmt, nd, false);
    case CLERI_GID_THING_BY_ID:
        return buf_append(&fmt->buf, nd->str, nd->len);
    case CLERI_GID_IMMUTABLE:
        nd = nd->children->node;
        switch (nd->cl_obj->gid)
        {
        case CLERI_GID_T_CLOSURE:
            return fmt__closure(fmt, nd);
        default:
            return buf_append(&fmt->buf, nd->str, nd->len);
        }
    }

    assert (0);
    return -1;
}

static int fmt__expression(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child;
    assert (nd->cl_obj->gid == CLERI_GID_EXPRESSION);

    for (child = nd->children->node->children; child; child = child->next)
        if (buf_append_str(&fmt->buf, "!"))
            return -1;


    fmt__expr_choice(fmt, nd->children->next->node);

    /* index */
    if (nd->children->next->next->node->children &&
        fmt__index(fmt, nd->children->next->next->node))
        return -1;

    /* chain */
    if (nd->children->next->next->next &&
        fmt__chain(fmt, nd->children->next->next->next->node, false))
        return -1;

    return 0;
}

static int fmt__statement(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * node;
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT);

    node = nd->children->node;
    switch (node->cl_obj->gid)
    {
    case CLERI_GID_EXPRESSION:
        return fmt__expression(fmt, node);
//    case CLERI_GID_OPERATIONS:
//        return fmt__operations(node, nd->children, 0);
    }
    assert (0);
    return -1;
}

void ti_fmt_init(ti_fmt_t * fmt, int indent)
{
    buf_init(&fmt->buf);
    fmt->indent = indent;
}

void ti_fmt_clear(ti_fmt_t * fmt)
{
    free(fmt->buf.data);
}

int ti_fmt_nd(ti_fmt_t * fmt, cleri_node_t * nd)
{
    switch(nd->cl_obj->gid)
    {
    case CLERI_GID_T_CLOSURE:
        return fmt__closure(fmt, nd);
    }
    assert(0);  /* unsupported node to format */
    return -1;
}
