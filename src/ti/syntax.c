/*
 * ti/syntax.c
 */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <langdef/langdef.h>
#include <ti/syntax.h>
#include <ti/val.h>
#include <tiinc.h>
#include <util/logger.h>

#define SYNTAX__X(__ev, __q, __nd, __str, __fn, __ret)                      \
do if (__nd->len == strlen(__str) && !memcmp(__nd->str, __str, __nd->len))  \
{                                                                           \
    __nd->data = (void *) ((uintptr_t) __fn);                               \
    __ev(__q);                                                              \
    return __ret;                                                           \
} while(0)

/* set collection event, used for collection scope */
#define syntax__cev_fn(__q, __nd, __str, __fn, __ret) \
        SYNTAX__X(syntax__set_collection_event, __q, __nd, __str, __fn, __ret)

/* set thingsdb event, used for thingsdb scope */
#define syntax__tev_fn(__q, __nd, __str, __fn, __ret) \
        SYNTAX__X(syntax__set_thingsdb_event, __q, __nd, __str, __fn, __ret)

/* no event, used for collection scope */
#define syntax__nev_fn(__q, __nd, __str, __fn, __ret) \
        SYNTAX__X((void), __q, __nd, __str, __fn, __ret)

/* no event, used for node and thingsdb scope */
#define syntax__sev_fn(__q, __nd, __str, __fn, __ret) \
        SYNTAX__X((void), __q, __nd, __str, __fn, __ret)

static inline void syntax__set_collection_event(ti_syntax_t * syntax)
{
    syntax->flags |= syntax->flags & TI_SYNTAX_FLAG_COLLECTION
            ? TI_SYNTAX_FLAG_EVENT : 0;
}

static inline void syntax__set_thingsdb_event(ti_syntax_t * syntax)
{
    syntax->flags |= ~syntax->flags & TI_SYNTAX_FLAG_COLLECTION
            ? TI_SYNTAX_FLAG_EVENT : 0;
}


/* returns `true` if the arguments of the function need evaluation */
static _Bool syntax__map_fn(ti_syntax_t * q, cleri_node_t * nd)
{
    /* a function name has at least size 1 */
    switch (*nd->str)
    {
    case 'a':
        syntax__nev_fn(q, nd, "assert", TI_FN_ASSERT, true);
        break;
    case 'b':
        syntax__nev_fn(q, nd, "blob", TI_FN_BLOB, true);
        syntax__nev_fn(q, nd, "bool", TI_FN_BOOL, true);
        break;
    case 'c':
        syntax__sev_fn(q, nd, "collection", TI_FN_COLLECTION, true);
        syntax__sev_fn(q, nd, "collections", TI_FN_COLLECTIONS, false);
        syntax__sev_fn(q, nd, "counters", TI_FN_COUNTERS, true);
        break;
    case 'd':
        syntax__cev_fn(q, nd, "del", TI_FN_DEL, true);
        syntax__tev_fn(q, nd, "del_collection", TI_FN_DEL_COLLECTION, true);
        syntax__tev_fn(q, nd, "del_user", TI_FN_DEL_USER, true);
        break;
    case 'e':
        syntax__nev_fn(q, nd, "endswith", TI_FN_ENDSWITH, true);
        break;
    case 'f':
        syntax__nev_fn(q, nd, "filter", TI_FN_FILTER, true);
        syntax__nev_fn(q, nd, "find", TI_FN_FIND, true);
        syntax__nev_fn(q, nd, "findindex", TI_FN_FINDINDEX, true);
        break;
    case 'g':
        syntax__tev_fn(q, nd, "grant", TI_FN_GRANT, true);
        break;
    case 'h':
        syntax__nev_fn(q, nd, "has", TI_FN_HAS, true);
        syntax__nev_fn(q, nd, "hasprop", TI_FN_HASPROP, true);
        break;
    case 'i':
        syntax__nev_fn(q, nd, "id", TI_FN_ID, false);
        syntax__nev_fn(q, nd, "indexof", TI_FN_INDEXOF, true);
        syntax__nev_fn(q, nd, "int", TI_FN_INT, true);
        syntax__nev_fn(q, nd, "isarray", TI_FN_ISARRAY, true);
        syntax__nev_fn(q, nd, "isascii", TI_FN_ISASCII, true);
        syntax__nev_fn(q, nd, "isbool", TI_FN_ISBOOL, true);
        syntax__nev_fn(q, nd, "isfloat", TI_FN_ISFLOAT, true);
        syntax__nev_fn(q, nd, "isinf", TI_FN_ISINF, true);
        syntax__nev_fn(q, nd, "isint", TI_FN_ISINT, true);
        syntax__nev_fn(q, nd, "islist", TI_FN_ISLIST, true);
        syntax__nev_fn(q, nd, "isnan", TI_FN_ISNAN, true);
        syntax__nev_fn(q, nd, "isnil", TI_FN_ISNIL, true);
        syntax__nev_fn(q, nd, "israw", TI_FN_ISRAW, true);
        syntax__nev_fn(q, nd, "isstr", TI_FN_ISSTR, true);
        syntax__nev_fn(q, nd, "isthing", TI_FN_ISTHING, true);
        syntax__nev_fn(q, nd, "istuple", TI_FN_ISTUPLE, true);
        syntax__nev_fn(q, nd, "isutf8", TI_FN_ISUTF8, true);
        break;
    case 'l':
        syntax__nev_fn(q, nd, "len", TI_FN_LEN, false);
        syntax__nev_fn(q, nd, "lower", TI_FN_LOWER, true);
        break;
    case 'm':
        syntax__nev_fn(q, nd, "map", TI_FN_MAP, true);
        break;
    case 'n':
        syntax__nev_fn(q, nd, "now", TI_FN_NOW, false);
        syntax__sev_fn(q, nd, "node", TI_FN_NODE, true);
        syntax__sev_fn(q, nd, "nodes", TI_FN_NODES, false);
        syntax__tev_fn(q, nd, "new_collection", TI_FN_NEW_COLLECTION, true);
        syntax__tev_fn(q, nd, "new_node", TI_FN_NEW_NODE, true);
        syntax__tev_fn(q, nd, "new_user", TI_FN_NEW_USER, true);
        break;
    case 'p':
        syntax__cev_fn(q, nd, "pop", TI_FN_POP, true);
        syntax__cev_fn(q, nd, "push", TI_FN_PUSH, true);
        syntax__tev_fn(q, nd, "pop_node", TI_FN_POP_NODE, false);
        break;
    case 'r':
        syntax__cev_fn(q, nd, "remove", TI_FN_REMOVE, true);
        syntax__cev_fn(q, nd, "rename", TI_FN_RENAME, true);
        syntax__nev_fn(q, nd, "refs", TI_FN_REFS, true);
        syntax__nev_fn(q, nd, "ret", TI_FN_RET, false);
        syntax__sev_fn(q, nd, "reset_counters", TI_FN_RESET_COUNTERS, false);
        syntax__tev_fn(q, nd, "rename_collection", TI_FN_RENAME_COLLECTION, true);
        syntax__tev_fn(q, nd, "rename_user", TI_FN_RENAME_USER, true);
        syntax__tev_fn(q, nd, "replace_node", TI_FN_REPLACE_NODE, true);
        syntax__tev_fn(q, nd, "revoke", TI_FN_REVOKE, true);
        break;
    case 's':
        syntax__cev_fn(q, nd, "splice", TI_FN_SPLICE, true);
        syntax__nev_fn(q, nd, "set", TI_FN_SET, true);
        syntax__nev_fn(q, nd, "startswith", TI_FN_STARTSWITH, true);
        syntax__nev_fn(q, nd, "str", TI_FN_STR, true);
        syntax__sev_fn(q, nd, "set_loglevel", TI_FN_SET_LOGLEVEL, true);
        syntax__sev_fn(q, nd, "set_zone", TI_FN_SET_ZONE, true);
        syntax__sev_fn(q, nd, "shutdown", TI_FN_SHUTDOWN, true);
        syntax__tev_fn(q, nd, "set_password", TI_FN_SET_PASSWORD, true);
        syntax__tev_fn(q, nd, "set_quota", TI_FN_SET_QUOTA, true);
        break;
    case 't':
        syntax__nev_fn(q, nd, "t", TI_FN_T, true);
        syntax__nev_fn(q, nd, "test", TI_FN_TEST, true);
        syntax__nev_fn(q, nd, "try", TI_FN_TRY, true);
        break;
    case 'u':
        syntax__nev_fn(q, nd, "upper", TI_FN_UPPER, true);
        syntax__sev_fn(q, nd, "user", TI_FN_USER, true);
        syntax__sev_fn(q, nd, "users", TI_FN_USERS, false);
        break;
    }

    nd->cl_obj->gid = TI_FN_0;  /* unknown function */
    return false;
}

static _Bool syntax__swap_opr(
        ti_syntax_t * syntax,
        cleri_children_t * parent,
        uint32_t parent_gid)
{
    cleri_node_t * node;
    cleri_node_t * nd = parent->node;
    cleri_children_t * chilcollection;
    uint32_t gid;

    assert( nd->cl_obj->tp == CLERI_TP_RULE ||
            nd->cl_obj->tp == CLERI_TP_PRIO ||
            nd->cl_obj->tp == CLERI_TP_THIS);

    node = nd->cl_obj->tp == CLERI_TP_PRIO ?
            nd->children->node :
            nd->children->node->children->node;

    if (node->cl_obj->gid == CLERI_GID_SCOPE)
    {
        ti_syntax_investigate(syntax, node);
        return false;
    }

    assert (node->cl_obj->tp == CLERI_TP_SEQUENCE);

    gid = node->children->next->node->children->node->cl_obj->gid;

    assert (gid >= CLERI_GID_OPR0_MUL_DIV_MOD && gid <= CLERI_GID_OPR7_CMP_OR);

    chilcollection = node->children->next->next;

    (void) syntax__swap_opr(syntax, node->children, gid);
    if (syntax__swap_opr(syntax, chilcollection, gid))
    {
        cleri_node_t * tmp;
        cleri_children_t * bchilda;
        parent->node = chilcollection->node;
        tmp = chilcollection->node->cl_obj->tp == CLERI_TP_PRIO ?
                chilcollection->node->children->node :
                chilcollection->node->children->node->children->node;
        bchilda = tmp->children;
        gid = tmp->cl_obj->gid;
        tmp = bchilda->node;
        bchilda->node = nd;
        chilcollection->node = tmp;
    }

    return gid > parent_gid;
}

static void syntax__investigate_array(ti_syntax_t * syntax, cleri_node_t * nd)
{
    uintptr_t sz = 0;
    cleri_children_t * child = nd          /* sequence */
            ->children->next->node         /* list */
            ->children;
    for (; child; child = child->next->next)
    {
        ti_syntax_investigate(syntax, child->node);
        ++sz;

        if (!child->next)
            break;
    }
    nd->data = (void *) sz;
}

void ti_syntax_investigate(ti_syntax_t * syntax, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        syntax__investigate_array(syntax, nd);
        return;
    case CLERI_GID_FUNCTION:
        if (syntax__map_fn(syntax, nd->children->node))
            /* investigate arguments */
            ti_syntax_investigate(syntax, nd->children->next->next->node);
        return;
    case CLERI_GID_ASSIGNMENT:
        syntax__set_collection_event(syntax);
        /* skip to scope */
        ti_syntax_investigate(syntax, nd->children->next->next->node);
        return;
    case CLERI_GID_CLOSURE:
        {
            uint8_t flags = syntax->flags;

            /* temporary remove the `event` flag to discover if the closure
             * sets the `event` flag.
             */
            syntax->flags &= ~TI_SYNTAX_FLAG_EVENT;

            /* investigate the scope, the rest can be skipped */
            ti_syntax_investigate(
                    syntax,
                    nd->children->next->next->next->node);

            nd->data = (void *) ((uintptr_t) (
                    syntax->flags & TI_SYNTAX_FLAG_EVENT
                        ? TI_VFLAG_CLOSURE_QBOUND|TI_VFLAG_CLOSURE_WSE
                        : TI_VFLAG_CLOSURE_QBOUND));

            /* apply the original flags */
            syntax->flags |= flags;
        }
        return;
    case CLERI_GID_COMMENT:
        /* all with children we can skip */
        return;
    case CLERI_GID_PRIMITIVES:
        switch (nd->children->node->cl_obj->gid)
        {
            case CLERI_GID_T_INT:
            case CLERI_GID_T_FLOAT:
            case CLERI_GID_T_STRING:
            case CLERI_GID_T_REGEX:
                ++syntax->nd_val_cache_n;
                nd->children->node->data = NULL;    /* init data to null */
        }
        return;
    case CLERI_GID_OPERATIONS:
        (void) syntax__swap_opr(syntax, nd->children->next, 0);
        if (nd->children->next->next->next)             /* optional (seq) */
        {
            cleri_children_t * child = \
                    nd->children->next->next->next->node->children->next;
            ti_syntax_investigate(syntax, child->node);
            ti_syntax_investigate(syntax, child->next->next->node);
        }
        return;
    }

    for (cleri_children_t * child = nd->children; child; child = child->next)
        ti_syntax_investigate(syntax, child->node);
}

void ti_syntax_init(ti_syntax_t * syntax)
{
    syntax->nd_val_cache_n = 0;
    syntax->flags = 0;

    /*
     * Properties `deep` and `pkg_id` are only used by `ti_query_t` and
     * are not initialized here.
     */
}
