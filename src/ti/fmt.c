/*
 * ti/fmt.c
 */
#include <assert.h>
#include <ctype.h>
#include <ti/fmt.h>
#include <langdef/langdef.h>
#include <util/logger.h>

#define INDENT_THRESHOLD 40

static int fmt__statement(ti_fmt_t * fmt, cleri_node_t * nd);

static inline int fmt__indent_str(ti_fmt_t * fmt, const char * str)
{
    return ti_fmt_indent(fmt) || buf_append_str(&fmt->buf, str);
}

static inline _Bool fmt__has_next_chain(cleri_node_t * nd)
{
    cleri_node_t * child = nd->children->next->next->next;
    return  child && child->children->next->children->next;
}

static int fmt__index(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * child = nd->children;
    do
    {
        cleri_node_t * c = child        /* sequence */
                ->children->next        /* slice */
                ->children;

        if (buf_write(&fmt->buf, '['))
            return -1;

        for (; c; c = c->next)
        {
            if (c->cl_obj->gid == CLERI_GID_STATEMENT)
                fmt__statement(fmt, c);
            else
                buf_write(&fmt->buf, ':');
        }

        if (buf_write(&fmt->buf, ']'))
            return -1;

        if ((c = child->children->next->next->next) && (
            buf_append_fmt(
                    &fmt->buf,
                    " %.*s ",
                    c->children->len,
                    c->children->str) ||
            fmt__statement(
                    fmt,
                    c->children->next)))
            return -1;
    }
    while ((child = child->next));

    return 0;
}

static int fmt__closure(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * first, * child;

    if (buf_write(&fmt->buf, '|'))
        return -1;

    first = nd                      /* sequence */
            ->children->next        /* list */
            ->children;             /* first child */

    for(child = first; child; child = child->next ? child->next->next : NULL)
    {
        if (child != first)
            if (buf_append_str(&fmt->buf, ", "))
                return -1;

        if (buf_append(&fmt->buf, child->str, child->len))
            return -1;
    }

    if (buf_append_str(&fmt->buf, "| "))
        return -1;

    return fmt__statement(fmt, nd->children->next->next->next);
}

static int fmt__function(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * child = nd->children;

    /* function name */
    if (buf_append(&fmt->buf, child->str, child->len) ||
        buf_write(&fmt->buf, '('))
        return -1;

    /* list (arguments) */
    child = child->next->children->next->children;

    while(child)
    {
        if (fmt__statement(fmt, child))
            return -1;

        if (!(child = child->next ? child->next->next : NULL))
            break;

        if (buf_append_str(&fmt->buf, ", "))
            return -1;
    }

    return buf_write(&fmt->buf, ')');
}

static int fmt__enum(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * name_nd = nd             /* sequence */
            ->children;                     /* name */
    nd = nd                                 /* sequence */
            ->children->next                /* enum node */
            ->children->next;               /* name or closure */

    /* enum name */
    if (buf_append(&fmt->buf, name_nd->str, name_nd->len) ||
        buf_write(&fmt->buf, '{'))
        return -1;

    switch(nd->cl_obj->gid)
    {
    case CLERI_GID_NAME:
        if (buf_append(&fmt->buf, nd->str, nd->len))
            return -1;
        break;

    case CLERI_GID_CLOSURE:
        if (fmt__closure(fmt, nd))
            return -1;
        break;
    }

    return buf_write(&fmt->buf, '}');
}


static int fmt__thing(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * child = nd           /* sequence */
            ->children->next            /* list */
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
            key = child->children;
            val = child->children->next->next;

            if (ti_fmt_indent(fmt) ||
                buf_append(&fmt->buf, key->str, key->len) ||
                buf_write(&fmt->buf, ':'))
                return 1;

            if (val && (
                buf_write(&fmt->buf, ' ') ||
                fmt__statement(fmt, val)))
                return -1;

            if (buf_append_str(&fmt->buf, ",\n"))
                return -1;
        }
        while (child->next && (child = child->next->next));
        --fmt->indent;

        return fmt__indent_str(fmt, "}");
    }

    key = child ? child->children : NULL;
    val = child ? child->children->next->next : NULL;

    if (buf_write(&fmt->buf, '{') ||
        (child && (
                buf_append(&fmt->buf, key->str, key->len) ||
                buf_append_str(&fmt->buf, ": ") ||
                (val && fmt__statement(fmt, val))  /* bug #334 */
        )) ||
        buf_write(&fmt->buf, '}')
    ) return -1;

    return 0;
}

static int fmt__var_opt_fa(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * child = nd->children;

    if (child->next)
    {
        switch(child->next->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            return fmt__function(fmt, nd);
        case CLERI_GID_ASSIGN:
            if (buf_append(&fmt->buf, child->str, child->len) ||
                buf_append_fmt(
                        &fmt->buf,
                        " %.*s ",
                        child->next->children->len,
                        child->next->children->str) ||
                fmt__statement(fmt, child->next->children->next))
                return -1;
            break;
        case CLERI_GID_INSTANCE:
            if (buf_append(&fmt->buf, child->str, child->len) ||
                fmt__thing(fmt, child->next))
                return -1;
            break;
        case CLERI_GID_ENUM_:
            return fmt__enum(fmt, nd);
        }
        return 0;
    }
    return buf_append(&fmt->buf, child->str, child->len);
}

static int fmt__name_opt_fa(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * child = nd->children;

    if (child->next)
    {
        switch(child->next->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            return fmt__function(fmt, nd);
        case CLERI_GID_ASSIGN:
            if (buf_append(&fmt->buf, child->str, child->len) ||
                buf_append_fmt(
                        &fmt->buf,
                        " %.*s ",
                        child->next->children->len,
                        child->next->children->str) ||
                fmt__statement(fmt, child->next->children->next))
                return -1;
            break;
        }
        return 0;
    }
    return buf_append(&fmt->buf, child->str, child->len);
}


static int fmt__chain(ti_fmt_t * fmt, cleri_node_t * nd, _Bool with_indent)
{
    cleri_node_t * child = nd->children->next;
    _Bool opt = nd->children->len == 2;
    _Bool set_indent = false;

    if (!with_indent && fmt__has_next_chain(nd) && nd->len > INDENT_THRESHOLD)
    {
        set_indent = true;
        ++fmt->indent;
    }

    if (with_indent)
    {
        if (buf_write(&fmt->buf, '\n') ||
            fmt__indent_str(fmt, opt ? ".." : "."))
            return -1;
    }
    else if (buf_write(&fmt->buf, '.') || (opt && buf_write(&fmt->buf, '.')))
        return -1;

    if (fmt__name_opt_fa(fmt, child))
        return -1;

    /* index */
    if ((child = child->next)->children && fmt__index(fmt, child))
        return -1;

    /* chain */
    if (child->next && fmt__chain(
            fmt,
            child->next,
            with_indent||set_indent))
        return -1;

    if (set_indent)
        --fmt->indent;

    return 0;
}

static int fmt__array(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * child = nd           /* sequence */
            ->children->next            /* list */
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
            if (ti_fmt_indent(fmt) ||
                fmt__statement(fmt, child) ||
                buf_append_str(&fmt->buf, ",\n"))
                return -1;
        }
        while (child->next && (child = child->next->next));
        --fmt->indent;

        return fmt__indent_str(fmt, "]");
    }


    if (buf_write(&fmt->buf, '[') ||
        (child && fmt__statement(fmt, child)) ||
        buf_write(&fmt->buf, ']'))
        return -1;

    return 0;
}

static int fmt__block(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * child = nd           /* seq<token({), list, }> */
            ->children->next            /* list statements */
            ->children;                 /* first child, not empty */


    /* Blocks are guaranteed to have at least one statement, otherwise it
     * would be a `thing` instead.
     */
    if (buf_append_str(&fmt->buf, "{\n"))
        return -1;

    ++fmt->indent;
    do
    {
        if (ti_fmt_indent(fmt) ||
            fmt__statement(fmt, child) ||
            buf_append_str(&fmt->buf, ";\n"))
            return -1;
    }
    while (child->next && (child = child->next->next));

    --fmt->indent;

    return fmt__indent_str(fmt, "}");
}

static int fmt__parenthesis(ti_fmt_t * fmt, cleri_node_t * nd)
{
    if (buf_write(&fmt->buf, '(') ||
        fmt__statement(fmt, nd->children->next) ||
        buf_write(&fmt->buf, ')'))
        return -1;
    return 0;
}

static int fmt__expr_choice(ti_fmt_t * fmt, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        return fmt__chain(fmt, nd, false);
    case CLERI_GID_T_FALSE:
    case CLERI_GID_T_FLOAT:
    case CLERI_GID_T_INT:
    case CLERI_GID_T_NIL:
    case CLERI_GID_T_REGEX:
    case CLERI_GID_T_STRING:
    case CLERI_GID_T_TRUE:
    case CLERI_GID_TEMPLATE:
        return buf_append(&fmt->buf, nd->str, nd->len);
    case CLERI_GID_VAR_OPT_MORE:
        return fmt__var_opt_fa(fmt, nd);
    case CLERI_GID_THING:
        return fmt__thing(fmt, nd);
    case CLERI_GID_ARRAY:
        return fmt__array(fmt, nd);
    case CLERI_GID_PARENTHESIS:
        return fmt__parenthesis(fmt, nd);
    }

    assert(0);
    return -1;
}

static int fmt__preopr(ti_fmt_t * fmt, cleri_node_t * nd)
{
    size_t n = nd->len;
    const char * s = nd->str;
    for(; n; --n, ++s)
        if (!isspace(*s) && buf_write(&fmt->buf, *s))
            return -1;
    return 0;
}

static int fmt__expression(ti_fmt_t * fmt, cleri_node_t * nd)
{
    if (fmt__preopr(fmt, nd->children))
        return -1;

    if (fmt__expr_choice(fmt, nd->children->next))
        return -1;

    /* index */
    if (nd->children->next->next->children &&
        fmt__index(fmt, nd->children->next->next))
        return -1;

    /* chain */
    if (nd->children->next->next->next &&
        fmt__chain(fmt, nd->children->next->next->next, false))
        return -1;

    return 0;
}

static int fmt__operations(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * node;

    if (nd->children->next->cl_obj->gid == CLERI_GID_OPR8_TERNARY)
    {
        cleri_node_t
            * nd_true = nd->children->next->children->next,
            * nd_false = nd->children->next->next;

        if (fmt__statement(fmt, nd->children))
            return -1;

        if (nd->len > INDENT_THRESHOLD)
        {
            ++fmt->indent;

            /* with indent */
            if (buf_write(&fmt->buf, '\n') ||
                fmt__indent_str(fmt, "? ") ||
                fmt__statement(fmt, nd_true) ||
                buf_write(&fmt->buf, '\n') ||
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

    node = nd->children->next;
    if (fmt__statement(fmt, nd->children) ||
        buf_append_fmt(&fmt->buf, " %.*s ", node->len, node->str) ||
        fmt__statement(fmt, nd->children->next->next))
        return -1;
    return 0;
}

static int fmt__if_statement(ti_fmt_t * fmt, cleri_node_t * nd)
{
    assert(nd->cl_obj->gid == CLERI_GID_IF_STATEMENT);

    if (buf_append_str(&fmt->buf, "if (") ||
        fmt__statement(fmt, nd->children->next->next) ||
        buf_append_str(&fmt->buf, ") ") ||
        fmt__statement(fmt, nd->children->data))
        return -1;

    return nd->children->next->data
        ? (buf_append_str(&fmt->buf, " else ") ||
            fmt__statement(fmt, nd->children->next->data))
        : 0;
}

static int fmt__return_statement(ti_fmt_t * fmt, cleri_node_t * nd)
{
    if (buf_append_str(&fmt->buf, "return ") ||
            fmt__statement(fmt, nd->children->next->children))
        return -1;

    nd = (cleri_node_t *) nd->children->data;

    if (!nd)
        return 0;

    if ((buf_append_str(&fmt->buf, ", ") || fmt__statement(fmt, nd)))
        return -1;

    return nd->next
        ? (buf_append_str(&fmt->buf, ", ") ||
                fmt__statement(fmt, nd->next->next))
        : 0;
}

static int fmt__for_statement(ti_fmt_t * fmt, cleri_node_t * nd)
{
    cleri_node_t * tmp, * child = nd->
            children->              /* for  */
            next->                  /* (    */
            next;                   /* List(variable) */

    if (buf_append_str(&fmt->buf, "for ("))
        return -1;

    tmp = child->children;
    do
    {
        if (buf_append(&fmt->buf, tmp->str, tmp->len))
            return -1;

        if (!(tmp = tmp->next ? tmp->next->next : NULL))
            break;

        if (buf_append_str(&fmt->buf, ", "))
            return -1;
    }
    while(1);

    if (buf_append_str(&fmt->buf, " in ") ||
        fmt__statement(fmt, (child = child->next->next)) ||
        buf_append_str(&fmt->buf, ") ") ||
        fmt__statement(fmt, (child = child->next->next)))
        return -1;

    return 0;
}

static int fmt__statement(ti_fmt_t * fmt, cleri_node_t * nd)
{
    assert(nd->cl_obj->gid == CLERI_GID_STATEMENT);

    nd = nd->children;
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_IF_STATEMENT:
        return fmt__if_statement(fmt, nd);
    case CLERI_GID_RETURN_STATEMENT:
        return fmt__return_statement(fmt, nd);
    case CLERI_GID_FOR_STATEMENT:
        return fmt__for_statement(fmt, nd);
    case CLERI_GID_K_CONTINUE:
        return buf_append_str(&fmt->buf, "continue");
    case CLERI_GID_K_BREAK:
        return buf_append_str(&fmt->buf, "break");
    case CLERI_GID_CLOSURE:
        return fmt__closure(fmt, nd);
    case CLERI_GID_BLOCK:
        return fmt__block(fmt, nd);
    case CLERI_GID_EXPRESSION:
        return fmt__expression(fmt, nd);
    case CLERI_GID_OPERATIONS:
        return fmt__operations(fmt, nd);
    }
    assert(0);
    return -1;
}

void ti_fmt_init(ti_fmt_t * fmt, int spaces)
{
    buf_init(&fmt->buf);
    fmt->spaces = spaces;
    fmt->indent = 0;
}

void ti_fmt_clear(ti_fmt_t * fmt)
{
    free(fmt->buf.data);
}

/*
 * Only `closure` nodes are supported but may be extend to other type
 */
int ti_fmt_nd(ti_fmt_t * fmt, cleri_node_t * nd)
{
    switch(nd->cl_obj->gid)
    {
    case CLERI_GID_CLOSURE:
        return fmt__closure(fmt, nd);
    }
    assert(0);  /* unsupported node to format */
    return -1;
}

int ti_fmt_ti_string(ti_fmt_t * fmt, ti_raw_t * raw)
{
    int sq = 0, dq= 0;
    char * c, * e, * data = (char *) raw->data;

    for (c = data, e = data + raw->n; c < e; ++c)
        if (*c == '\'')
            ++sq;
        else if (*c == '"')
            ++dq;

    /*
     * First try single quotes if no single quotes exist in the string
     */
    if (!sq)
        return (
            buf_write(&fmt->buf, '\'') ||
            buf_append(&fmt->buf, (const char *) raw->data, raw->n) ||
            buf_write(&fmt->buf, '\''));

    /*
     * Try double quotes since at least one single quote exist in the string
     */
    if (!dq)
        return (
            buf_write(&fmt->buf, '"') ||
            buf_append(&fmt->buf, (const char *) raw->data, raw->n) ||
            buf_write(&fmt->buf, '"'));

    /*
     * Fall-back to single quotes with escaped single quotes inside the string
     */
    if (buf_write(&fmt->buf, '\''))
        return -1;

    for (c = data, e = data + raw->n; c < e; ++c)
        if (*c == '\'')
        {
            if (buf_append_str(&fmt->buf, "''"))
                return -1;
        }
        else if (buf_write(&fmt->buf, *c))
            return -1;

    return buf_write(&fmt->buf, '\'');
}
