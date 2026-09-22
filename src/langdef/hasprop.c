/*
 * langdef/utils.c
 */

#include <langdef/langdef.h>
#include <stdio.h>

static inline void hasprop__return_statement(
        ti_hasprop_t * hasprop,
        cleri_node_t * nd)
{
    cleri_node_t * node = nd->children->next->children;
    if (!node)
    {
        nd->data = ti_do_return_nil;
        nd->children->data = NULL;
        return;
    }

    hasprop__statement(hasprop, node);
    if (node->next)
    {
        node = nd->children->data = node->next->next;
        hasprop__statement(hasprop, node);
        if (node->next)
        {
            hasprop__statement(hasprop, node->next->next);
            nd->data = ti_do_return_alt_flags;
        }
        else
        {
            nd->data = ti_do_return_alt_deep;
        }
    }
    else
    {
        nd->data = ti_do_return_val;
        nd->children->data = NULL;
    }
}

static inline _Bool hasprop__if_statement(ti_hasprop_t * hasprop, cleri_node_t * nd)
{
    hasprop__statement(hasprop, nd->children->next->next);
    hasprop__statement(hasprop, nd->children->data);

    /* set else node */
    if (nd->children->next->data)
        hasprop__statement(hasprop, nd->children->next->data);
}

_Bool langdef_hasprop(cleri_node_t * nd, char * str, size_t n)
{
    assert(nd->cl_obj->gid == CLERI_GID_STATEMENT);

    switch (nd->children->cl_obj->gid)
    {
    case CLERI_GID_IF_STATEMENT:
        hasprop__if_statement(hasprop, nd->children);
        return;
    case CLERI_GID_RETURN_STATEMENT:
        hasprop__return_statement(hasprop, nd->children);
        return;
    case CLERI_GID_FOR_STATEMENT:
        hasprop__for_statement(hasprop, nd->children);
        return;
    case CLERI_GID_K_CONTINUE:
        if (~hasprop->flags & TI_QBIND_FLAG_FOR_LOOP)
            hasprop->flags |= TI_QBIND_FLAG_ILL_CONTINUE;
        nd->children->data = ti_do_continue;
        return;
    case CLERI_GID_K_BREAK:
        if (~hasprop->flags & TI_QBIND_FLAG_FOR_LOOP)
            hasprop->flags |= TI_QBIND_FLAG_ILL_BREAK;
        nd->children->data = ti_do_break;
        return;
    case CLERI_GID_CLOSURE:
        hasprop__closure(hasprop, nd->children);
        return;
    case CLERI_GID_EXPRESSION:
        hasprop->flags &= ~TI_QBIND_FLAG_ON_VAR;
        hasprop__expression(hasprop, nd->children);
        return;
    case CLERI_GID_BLOCK:
        hasprop__block(hasprop, nd->children);
        return;
    case CLERI_GID_OPERATIONS:
        hasprop__operations(hasprop, &nd->children, 0);
    }
}