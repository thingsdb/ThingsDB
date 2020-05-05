/*
 * ti/fmt.c
 */
#include <assert.h>
#include <ti/fmt.h>
#include <langdef/langdef.h>
#include <util/logger.h>

#define INDENT_THRESHOLD 40

static int fmt__statement(ti_fmt_t * fmt, cleri_node_t * nd);

static inline int fmt__indent(ti_fmt_t * fmt)
{
    int spaces = fmt->indent * fmt->indent_spaces;
    return spaces
            ? buf_append_fmt(&fmt->buf, "%*s", spaces, "")
            : 0;
}

static inline int fmt__indent_str(ti_fmt_t * fmt, const char * str)
{
    int spaces = fmt->indent * fmt->indent_spaces;
    return buf_append_fmt(&fmt->buf, "%*s%s", spaces, "", str);
}

static inline _Bool fmt__chain_is_func(cleri_node_t * nd)
{
    cleri_children_t * child = nd->children->next->node->children->next;
    return  child && child->node->cl_obj->gid == CLERI_GID_FUNCTION;
}

static inline _Bool fmt__chain_next_is_func(cleri_node_t * nd)
{
    cleri_children_t * child = nd->children->next->next->next;
    return  child && fmt__chain_is_func(child->node);
}

static inline _Bool fmt__has_next_chain(cleri_node_t * nd)
{
    cleri_children_t * child = nd->children->next->next->next;
    return  child && child->node->children->next->node->children->next;
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

    return fmt__statement(fmt, nd->children->next->next->next->node);
}

static int fmt__function(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child = nd->children;

    /* function name */
    if (buf_append(&fmt->buf, child->node->str, child->node->len) ||
        buf_append_str(&fmt->buf, "("))
        return -1;

    /* list (arguments) */
    child = child->next->node->children->next->node->children;

    while(child)
    {
        if (fmt__statement(fmt, child->node))
            return -1;

        if (!(child = child->next ? child->next->next : NULL))
            break;

        if (buf_append_str(&fmt->buf, ", "))
            return -1;
    }

    return buf_append_str(&fmt->buf, ")");
}

static int fmt__thing(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child = nd          /* sequence */
            ->children->next->node         /* list */
            ->children;
    cleri_node_t * key, * val;

    /* Use indentation if more than 1 element */
    _Bool with_indent = child && child->next && child->next->next;

    if (with_indent)
    {
        if (buf_append_str(&fmt->buf, "{\n"))
            return -1;
        ++fmt->indent;
        do
        {
            key = child->node->children->node;
            val =  child->node->children->next->next->node;

            if (fmt__indent(fmt) ||
                buf_append(&fmt->buf, key->str, key->len) ||
                buf_append_str(&fmt->buf, ": ") ||
                fmt__statement(fmt, val) ||
                buf_append_str(&fmt->buf, ",\n"))
                return -1;
        }
        while (child->next && (child = child->next->next));
        --fmt->indent;

        return fmt__indent_str(fmt, "}");
    }

    key = child ? child->node->children->node : NULL;
    val = child ? child->node->children->next->next->node : NULL;

    if (buf_append_str(&fmt->buf, "{") ||
        (child && (
                buf_append(&fmt->buf, key->str, key->len) ||
                buf_append_str(&fmt->buf, ":") ||
                fmt__statement(fmt, val)
        )) ||
        buf_append_str(&fmt->buf, "}")
    ) return -1;

    return 0;
}

static int fmt__var_opt_fa(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child = nd->children;

    if (child->next)
    {
        switch(child->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            return fmt__function(fmt, nd);
        case CLERI_GID_ASSIGN:
            if (buf_append(&fmt->buf, child->node->str, child->node->len) ||
                buf_append_str(&fmt->buf, " = ") ||
                fmt__statement(fmt, child->next->node->children->next->node))
                return -1;
            break;
        case CLERI_GID_INSTANCE:
            if (buf_append(&fmt->buf, child->node->str, child->node->len) ||
                fmt__thing(fmt, child->next->node))
                return -1;
            break;
        }
        return 0;
    }
    return buf_append(&fmt->buf, child->node->str, child->node->len);
}

static int fmt__name_opt_fa(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child = nd->children;

    if (child->next)
    {
        switch(child->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            return fmt__function(fmt, nd);
        case CLERI_GID_ASSIGN:
            if (buf_append(&fmt->buf, child->node->str, child->node->len) ||
                buf_append_str(&fmt->buf, " = ") ||
                fmt__statement(fmt, child->next->node->children->next->node))
                return -1;
            break;
        }
        return 0;
    }
    return buf_append(&fmt->buf, child->node->str, child->node->len);
}


static int fmt__chain(ti_fmt_t * fmt, cleri_node_t * nd, _Bool with_indent)
{
    cleri_children_t * child = nd->children->next;
    _Bool set_indent = false;

    if (!with_indent && fmt__has_next_chain(nd) && nd->len > INDENT_THRESHOLD)
    {
        with_indent = set_indent = true;
        ++fmt->indent;
    }

    if (with_indent)
    {
        if (buf_append_str(&fmt->buf, "\n") ||
            fmt__indent_str(fmt, "."))
            return -1;
    }
    else if (buf_append_str(&fmt->buf, "."))
        return -1;

    if (fmt__name_opt_fa(fmt, child->node))
        return -1;

    /* index */
    if ((child = child->next)->node->children && fmt__index(fmt, child->node))
        return -1;

    /* chain */
    if (child->next && fmt__chain(fmt, child->next->node, with_indent))
        return -1;

    if (set_indent)
        --fmt->indent;

    return 0;
}

static int fmt__array(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child = nd          /* sequence */
            ->children->next->node         /* list */
            ->children;
    /* Use indentation if more than 1 element */
    _Bool with_indent = child && child->next && child->next->next;

    if (with_indent)
    {
        if (buf_append_str(&fmt->buf, "[\n"))
            return -1;
        ++fmt->indent;
        do
        {
            if (fmt__indent(fmt) ||
                fmt__statement(fmt, child->node) ||
                buf_append_str(&fmt->buf, ",\n"))
                return -1;
        }
        while (child->next && (child = child->next->next));
        --fmt->indent;

        return fmt__indent_str(fmt, "]");
    }


    if (buf_append_str(&fmt->buf, "[") ||
        (child && fmt__statement(fmt, child->node)) ||
        buf_append_str(&fmt->buf, "]"))
        return -1;

    return 0;
}

static int fmt__block(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_children_t * child = nd           /* seq<{, comment, list, }> */
            ->children->next->next->node    /* list statements */
            ->children;                     /* first child, not empty */


    /* Blocks are guaranteed to have at least one statement, otherwise it
     * would be a `thing` instead.
     */
    if (buf_append_str(&fmt->buf, "{\n"))
        return -1;

    ++fmt->indent;
    do
    {
        if (fmt__indent(fmt) ||
            fmt__statement(fmt, child->node) ||
            buf_append_str(&fmt->buf, ";\n"))
            return -1;
    }
    while (child->next && (child = child->next->next));

    --fmt->indent;

    return fmt__indent_str(fmt, "}");
}

static int fmt__parenthesis(ti_fmt_t * fmt, cleri_node_t * nd)
{
    if (buf_append_str(&fmt->buf, "(") ||
        fmt__statement(fmt, nd->children->next->node) ||
        buf_append_str(&fmt->buf, ")"))
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
        }
        return buf_append(&fmt->buf, nd->str, nd->len);
    case CLERI_GID_VAR_OPT_MORE:
        return fmt__var_opt_fa(fmt, nd);
    case CLERI_GID_THING:
        return fmt__thing(fmt, nd);
    case CLERI_GID_ARRAY:
        return fmt__array(fmt, nd);
    case CLERI_GID_BLOCK:
        return fmt__block(fmt, nd);
    case CLERI_GID_PARENTHESIS:
        return fmt__parenthesis(fmt, nd);
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

    if (fmt__expr_choice(fmt, nd->children->next->node))
        return -1;

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

static int fmt__operations(ti_fmt_t * fmt, cleri_node_t * nd)
{
    assert( nd->cl_obj->gid == CLERI_GID_OPERATIONS );
    assert (nd->cl_obj->tp == CLERI_TP_SEQUENCE);


    switch (nd->children->next->node->cl_obj->gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_AND:
    case CLERI_GID_OPR3_BITWISE_XOR:
    case CLERI_GID_OPR4_BITWISE_OR:
    case CLERI_GID_OPR5_COMPARE:
    case CLERI_GID_OPR6_CMP_AND:
    case CLERI_GID_OPR7_CMP_OR:
    {
        cleri_node_t * node;
        node = nd->children->next->node;
        if (fmt__statement(fmt, nd->children->node) ||
            buf_append_fmt(&fmt->buf, " %.*s ", (int) node->len, node->str) ||
            fmt__statement(fmt, nd->children->next->next->node))
            return -1;
        return 0;
    }
    case CLERI_GID_OPR8_TERNARY:
    {
        cleri_node_t
            * nd_true = nd->children->next->node->children->next->node,
            * nd_false = nd->children->next->next->node;

        if (fmt__statement(fmt, nd->children->node))
            return -1;

        if (nd->len > INDENT_THRESHOLD)
        {
            ++fmt->indent;

            /* with indent */
            if (buf_append_str(&fmt->buf, "\n") ||
                fmt__indent_str(fmt, "? ") ||
                fmt__statement(fmt, nd_true) ||
                buf_append_str(&fmt->buf, "\n") ||
                fmt__indent_str(fmt, ": ") ||
                fmt__statement(fmt, nd_false)
            ) return -1;

            --fmt->indent;
            return 0;
        }

        if (buf_append_str(&fmt->buf, " ? ") ||
            fmt__statement(fmt, nd_true) ||
            buf_append_str(&fmt->buf, " : ") ||
            fmt__statement(fmt, nd_false))
            return -1;
        return 0;
    }
    }

    assert (0);
    return -1;
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
    case CLERI_GID_OPERATIONS:
        return fmt__operations(fmt, node);
    }
    assert (0);
    return -1;
}

void ti_fmt_init(ti_fmt_t * fmt, int indent_spaces)
{
    buf_init(&fmt->buf);
    fmt->indent_spaces = indent_spaces;
    fmt->indent = 0;
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
