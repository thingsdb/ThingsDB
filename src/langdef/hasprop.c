/*
 * langdef/hasprop.c
 */

#include <langdef/langdef.h>
#include <langdef/hasprop.h>
#include <stdio.h>

static inline _Bool hasprop__return_statement(cleri_node_t * nd,
                                              const char * str,
                                              const size_t n)
{
    cleri_node_t * node = nd->children->next->children;
    if (!node)
        return false;

    if (langdef_hasprop(node, str, n))
        return true;

    if (node->next)
    {
        node = node->next->next;
        if (langdef_hasprop(node, str, n))
            return true;

        if (node->next)
            return langdef_hasprop(node->next->next, str, n);
    }
    return false;
}

static inline _Bool hasprop__if_statement(cleri_node_t * nd,
                                          const char * str,
                                          const size_t n)
{
    if (langdef_hasprop(nd->children->next->next, str, n) ||
        langdef_hasprop(nd->children->data, str, n))
        return true;

    if (nd->children->next->data)
        /* else node */
        return langdef_hasprop(nd->children->next->data, str, n);
    return false;
}

static inline _Bool hasprop__for_statement(cleri_node_t * nd,
                                           const char * str,
                                           const size_t n)
{
    cleri_node_t * child = nd->
            children->              /* for  */
            next->                  /* (    */
            next;                   /* List(variable) */

    return (
        langdef_hasprop((child = child->next->next), str, n) ||
        langdef_hasprop((child = child->next->next), str, n)
    );
}

static _Bool hasprop__index(cleri_node_t * nd,
                            const char * str,
                            const size_t n)
{
    cleri_node_t * child = nd->children;
    do
    {
        cleri_node_t * c = child        /* sequence */
                ->children->next        /* slice */
                ->children;

        if (child->children->next->next->next &&
            langdef_hasprop(child                     /* sequence */
                    ->children->next->next->next      /* assignment */
                    ->children->next, str, n))        /* statement */
            return true;

        for (; c; c = c->next)
            if (c->cl_obj->gid == CLERI_GID_STATEMENT)
                if (langdef_hasprop(c, str, n))
                    return true;
    }
    while ((child = child->next));
    return false;
}

static inline _Bool hasprop__closure(cleri_node_t * nd,
                                     const char * str,
                                     const size_t n)
{

    cleri_node_t * child = node         /* sequence */
            ->children->next            /* list */
            ->children;                 /* first child */
    for (; child; child = child->next->next)
        if (child->len == n && memcmp(child->str, str, n) == 0)
            return false;  /* var is overwritten in closure */

    return langdef_hasprop(nd->children->next->next->next, str, n);
}

static inline _Bool hasprop__thing(cleri_node_t * nd,
                                   const char * str,
                                   const size_t n)
{
    cleri_node_t * child = nd           /* sequence */
            ->children->next            /* list */
            ->children;
    for (; child; child = child->next->next)
        if (langdef_hasprop(child->children->next->next, str, n))
            return true;
    return false;
}

static inline _Bool hasprop__function(cleri_node_t * nd,
                                      const char * str,
                                      const size_t n)
{
    /* list (arguments) */
    nd = nd->children->next->children->next;

    for(child = nd->children;
        child;
        child = child->next ? child->next->next : NULL)
    {
        if (langdef_hasprop(child, str, n))  /* statement */
            return true;
    }
    return false;
}

static inline _Bool hasprop__name_opt_fa(cleri_node_t * nd,
                                         const char * str,
                                         const size_t n)
{
    if (nd->children->next)
    {
        switch (nd->children->next->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            return hasprop__function(nd, str, n);
        case CLERI_GID_ASSIGN:
            return langdef_hasprop(nd->children->next->children->next, str, n);
        default:
            assert(0);
            return;
        }
    }
    return false;
}

static inline _Bool hasprop__enum(cleri_node_t * nd,
                                  const char * str,
                                  const size_t n)
{
    return ((nd = nd->children->next)->cl_obj->gid == CLERI_GID_CLOSURE &&
            hasprop__closure(nd, str, n));
}

static _Bool hasprop__var_opt_fa(cleri_node_t * nd,
                                 const char * str,
                                 const size_t n)
{
    if (nd->children->next)
    {
        switch (nd->children->next->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            if (hasprop__function(nd, str, n))
                return true;
            break;
        case CLERI_GID_ASSIGN:
            if (langdef_hasprop(nd->children->next->children->next, str, n))
                return true;
            if (nd->children->next->len == 1)
                return false;  /* create a new var so we can skip the check */
            break;
        case CLERI_GID_INSTANCE:
            return hasprop__thing(nd->children->next, str, n);
        case CLERI_GID_ENUM_:
            return hasprop__enum(nd->children->next, str, n);
        default:
            assert(0);
            return;
        }
    }
    return nd->children->len == n && memcmp(nd->children->str, str, n) == 0;
}

static inline _Bool hasprop__chain(cleri_node_t * nd,
                                   const char * str,
                                   const size_t n)
{
    cleri_node_t * child = nd->children->next;

    if (hasprop__name_opt_fa(child, str, n))
        return true;

    if ((child = child->next)->children)
        if (hasprop__index(child, str, n))
            return true;

    if (child->next)
        return hasprop__chain(child->next, str, n);

    return false;
}

static inline _Bool hasprop__expression(cleri_node_t * nd,
                                        const char * str,
                                        const size_t n)
{
    cleri_node_t * node = nd->children->next;

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        if (hasprop__chain(node, str, n))
            return true;
        break;
    case CLERI_GID_T_INT:
    case CLERI_GID_T_FLOAT:
    case CLERI_GID_T_STRING:
    case CLERI_GID_T_REGEX:
        break;
    case CLERI_GID_T_ANO:
        if (hasprop__thing(node, str, n))
            return true;
        break;
    case CLERI_GID_TEMPLATE:
    {
        cleri_node_t * child = node         /* sequence */
                ->children->next            /* repeat */
                ->children;
        for (; child; child = child->next)
        {
            if (child->cl_obj->tp == CLERI_TP_SEQUENCE &&
                langdef_hasprop(child->children->next, str, n))
                    return true;
        }
        break;
    }
    case CLERI_GID_VAR_OPT_MORE:
        if (hasprop__var_opt_fa(node, str, n))
            return true;
        break;
    case CLERI_GID_THING:
        if (hasprop__thing(node, str, n))
            return true;
        break;
    case CLERI_GID_ARRAY:
    {
        cleri_node_t * child = node     /* sequence */
                ->children->next        /* list */
                ->children;
        for (; child; child = child->next ? child->next->next : NULL)
            if (langdef_hasprop(child, str, n))  /* statement */
                return true;
        break;
    }
    case CLERI_GID_PARENTHESIS:
        if (langdef_hasprop(node->children->next, str, n))
            return true;
        break;
    }

    if (node->next->children)
        if (hasprop__index(node->next, str, n))
            return true;

    if (node->next->next)
        return hasprop__chain(node->next->next, str, n);

    return false;
}

static inline _Bool hasprop__block(cleri_node_t * nd,
                                   const char * str,
                                   const size_t n)
{
    cleri_node_t * child = nd       /* seq<token({), list, }> */
            ->children->next        /* list statements */
            ->children;             /* first child, not empty */
    do
    {
        if (langdef_hasprop(child, str, n))  /* statement */
            return true;
        if (!child->next)
            break;
    }
    while ((child = child->next->next));
    return false;
}

static _Bool hasprop__operations(cleri_node_t * nd,
                                 const char * str,
                                 const size_t n))
{
    uint32_t gid = nd->children->next->cl_obj->gid;
    cleri_node_t * childb = nd->children->next->next;

    if (langdef_hasprop(nd->children, str, n))
        return true;

    if (gid == CLERI_GID_OPR9_TERNARY)
        if (langdef_hasprop(nd->children->next->children->next, str, n))
            return true;

    if (childb->children->cl_obj->gid != CLERI_GID_OPERATIONS)
        if (langdef_hasprop(childb, str, n))
            return true;

    return false;
}

_Bool langdef_hasprop(cleri_node_t * nd, const char * str, const size_t n)
{
    assert(nd->cl_obj->gid == CLERI_GID_STATEMENT);

    switch (nd->children->cl_obj->gid)
    {
    case CLERI_GID_IF_STATEMENT:
        return hasprop__if_statement(nd->children, str, n);
    case CLERI_GID_RETURN_STATEMENT:
        return hasprop__return_statement(nd->children, str, n);
    case CLERI_GID_FOR_STATEMENT:
        return hasprop__for_statement(nd->children, str, n);
    case CLERI_GID_K_CONTINUE:
        return false;
    case CLERI_GID_K_BREAK:
        return false;
    case CLERI_GID_CLOSURE:
        return hasprop__closure(nd->children, str, n);
    case CLERI_GID_EXPRESSION:
        return hasprop__expression(nd->children, str, n);
    case CLERI_GID_BLOCK:
        hasprop__block(nd->children, str, n);
        return;
    case CLERI_GID_OPERATIONS:
        hasprop__operations(nd->children, str, n);
    }
}