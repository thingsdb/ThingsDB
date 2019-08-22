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

#define SYNTAX__X(__ev, __q, __nd, __str, __fn)                             \
do if (__nd->len == strlen(__str) && !memcmp(__nd->str, __str, __nd->len))  \
{                                                                           \
    __nd->data = (void *) ((uintptr_t) __fn);                               \
    __ev(__q);                                                              \
    return;                                                                 \
} while(0)

/* set 'c'ollection event, used for collection scope */
#define syntax__cev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X(syntax__set_collection_event, __q, __nd, __str, __fn)

/* 'n'o event, used for collection scope */
#define syntax__nev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X((void), __q, __nd, __str, __fn)

/* set 't'hingsdb event, used for thingsdb scope */
#define syntax__tev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X(syntax__set_thingsdb_event, __q, __nd, __str, __fn)
/* 'z'ero (no) event, used for node and thingsdb scope */
#define syntax__zev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X((void), __q, __nd, __str, __fn)

/* set 'b'oth thingsdb and collection event */
#define syntax__bev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X(syntax__set_both_event, __q, __nd, __str, __fn)

static inline void syntax__set_collection_event(ti_syntax_t * syntax)
{
    syntax->flags |= syntax->flags & TI_SYNTAX_FLAG_COLLECTION
            ? TI_SYNTAX_FLAG_EVENT : 0;
}

static inline void syntax__set_thingsdb_event(ti_syntax_t * syntax)
{
    syntax->flags |= syntax->flags & TI_SYNTAX_FLAG_THINGSDB
            ? TI_SYNTAX_FLAG_EVENT : 0;
}

static inline void syntax__set_both_event(ti_syntax_t * syntax)
{
    syntax->flags |= ~syntax->flags & TI_SYNTAX_FLAG_NODE
            ? TI_SYNTAX_FLAG_EVENT : 0;
}

static void syntax__map_fn(ti_syntax_t * q, cleri_node_t * nd)
{
    /* a function name has at least size 1 */
    switch ((ti_alpha_lower_t) *nd->str)
    {
    case 'a':
        syntax__cev_fn(q, nd, "add", TI_FN_ADD);
        syntax__nev_fn(q, nd, "array", TI_FN_ARRAY);
        syntax__nev_fn(q, nd, "assert", TI_FN_ASSERT);
        syntax__nev_fn(q, nd, "assert_err", TI_FN_ASSERT_ERR);
        syntax__nev_fn(q, nd, "auth_err", TI_FN_AUTH_ERR);
        break;
    case 'b':
        syntax__nev_fn(q, nd, "bad_data_err", TI_FN_BAD_DATA_ERR);
        syntax__nev_fn(q, nd, "blob", TI_FN_BLOB);
        syntax__nev_fn(q, nd, "bool", TI_FN_BOOL);
        break;
    case 'c':
        syntax__nev_fn(q, nd, "call", TI_FN_CALL);
        syntax__nev_fn(q, nd, "contains", TI_FN_CONTAINS);
        syntax__zev_fn(q, nd, "collection_info", TI_FN_COLLECTION_INFO);
        syntax__zev_fn(q, nd, "collections_info", TI_FN_COLLECTIONS_INFO);
        syntax__zev_fn(q, nd, "counters", TI_FN_COUNTERS);
        break;
    case 'd':
        syntax__cev_fn(q, nd, "del", TI_FN_DEL);  /* most frequent used */
        syntax__bev_fn(q, nd, "del_procedure", TI_FN_DEL_PROCEDURE);
        syntax__nev_fn(q, nd, "deep", TI_FN_DEEP);
        syntax__tev_fn(q, nd, "del_collection", TI_FN_DEL_COLLECTION);
        syntax__tev_fn(q, nd, "del_expired", TI_FN_DEL_EXPIRED);
        syntax__tev_fn(q, nd, "del_token", TI_FN_DEL_TOKEN);
        syntax__tev_fn(q, nd, "del_user", TI_FN_DEL_USER);
        break;
    case 'e':
        syntax__nev_fn(q, nd, "endswith", TI_FN_ENDSWITH);
        syntax__nev_fn(q, nd, "err", TI_FN_ERR);
        break;
    case 'f':
        syntax__nev_fn(q, nd, "filter", TI_FN_FILTER);
        syntax__nev_fn(q, nd, "find", TI_FN_FIND);
        syntax__nev_fn(q, nd, "findindex", TI_FN_FINDINDEX);
        syntax__nev_fn(q, nd, "float", TI_FN_FLOAT);
        syntax__nev_fn(q, nd, "forbidden_err", TI_FN_FORBIDDEN_ERR);
        break;
    case 'g':
        syntax__tev_fn(q, nd, "grant", TI_FN_GRANT);
        break;
    case 'h':
        syntax__nev_fn(q, nd, "has", TI_FN_HAS);
        syntax__nev_fn(q, nd, "hasprop", TI_FN_HASPROP);
        break;
    case 'i':
        syntax__nev_fn(q, nd, "id", TI_FN_ID);
        syntax__nev_fn(q, nd, "index_err", TI_FN_INDEX_ERR);
        syntax__nev_fn(q, nd, "indexof", TI_FN_INDEXOF);
        syntax__nev_fn(q, nd, "int", TI_FN_INT);
        syntax__nev_fn(q, nd, "isarray", TI_FN_ISARRAY);
        syntax__nev_fn(q, nd, "isascii", TI_FN_ISASCII);
        syntax__nev_fn(q, nd, "isbool", TI_FN_ISBOOL);
        syntax__nev_fn(q, nd, "iserr", TI_FN_ISERROR);
        syntax__nev_fn(q, nd, "isfloat", TI_FN_ISFLOAT);
        syntax__nev_fn(q, nd, "isinf", TI_FN_ISINF);
        syntax__nev_fn(q, nd, "isint", TI_FN_ISINT);
        syntax__nev_fn(q, nd, "islist", TI_FN_ISLIST);
        syntax__nev_fn(q, nd, "isnan", TI_FN_ISNAN);
        syntax__nev_fn(q, nd, "isnil", TI_FN_ISNIL);
        syntax__nev_fn(q, nd, "israw", TI_FN_ISRAW);
        syntax__nev_fn(q, nd, "isset", TI_FN_ISSET);
        syntax__nev_fn(q, nd, "isstr", TI_FN_ISSTR);
        syntax__nev_fn(q, nd, "isthing", TI_FN_ISTHING);
        syntax__nev_fn(q, nd, "istuple", TI_FN_ISTUPLE);
        syntax__nev_fn(q, nd, "isutf8", TI_FN_ISUTF8);
        break;
    case 'j':
        break;
    case 'k':
        syntax__nev_fn(q, nd, "keys", TI_FN_KEYS);
        break;
    case 'l':
        syntax__nev_fn(q, nd, "len", TI_FN_LEN);
        syntax__nev_fn(q, nd, "lower", TI_FN_LOWER);
        break;
    case 'm':
        syntax__nev_fn(q, nd, "map", TI_FN_MAP);
        syntax__nev_fn(q, nd, "max_quota_err", TI_FN_MAX_QUOTA_ERR);
        break;
    case 'n':
        syntax__bev_fn(q, nd, "new_procedure", TI_FN_NEW_PROCEDURE);
        syntax__nev_fn(q, nd, "node_err", TI_FN_NODE_ERR);
        syntax__nev_fn(q, nd, "now", TI_FN_NOW);
        syntax__tev_fn(q, nd, "new_collection", TI_FN_NEW_COLLECTION);
        syntax__tev_fn(q, nd, "new_node", TI_FN_NEW_NODE);
        syntax__tev_fn(q, nd, "new_token", TI_FN_NEW_TOKEN);
        syntax__tev_fn(q, nd, "new_user", TI_FN_NEW_USER);
        syntax__zev_fn(q, nd, "node_info", TI_FN_NODE_INFO);
        syntax__zev_fn(q, nd, "nodes_info", TI_FN_NODES_INFO);
        break;
    case 'o':
        syntax__nev_fn(q, nd, "overflow_err", TI_FN_OVERFLOW_ERR);
        break;
    case 'p':
        syntax__cev_fn(q, nd, "pop", TI_FN_POP);
        syntax__cev_fn(q, nd, "push", TI_FN_PUSH);
        syntax__nev_fn(q, nd, "procedure_doc", TI_FN_PROCEDURE_DOC);
        syntax__nev_fn(q, nd, "procedure_info", TI_FN_PROCEDURE_INFO);
        syntax__nev_fn(q, nd, "procedures_info", TI_FN_PROCEDURES_INFO);
        syntax__tev_fn(q, nd, "pop_node", TI_FN_POP_NODE);
        break;
    case 'q':
        break;
    case 'r':
        syntax__cev_fn(q, nd, "remove", TI_FN_REMOVE);
        syntax__nev_fn(q, nd, "raise", TI_FN_RAISE);
        syntax__nev_fn(q, nd, "refs", TI_FN_REFS);
        syntax__nev_fn(q, nd, "return", TI_FN_RETURN);
        syntax__nev_fn(q, nd, "run", TI_FN_RUN);
        syntax__tev_fn(q, nd, "rename_collection", TI_FN_RENAME_COLLECTION);
        syntax__tev_fn(q, nd, "rename_user", TI_FN_RENAME_USER);
        syntax__tev_fn(q, nd, "replace_node", TI_FN_REPLACE_NODE);
        syntax__tev_fn(q, nd, "revoke", TI_FN_REVOKE);
        syntax__zev_fn(q, nd, "reset_counters", TI_FN_RESET_COUNTERS);
        break;
    case 's':
        syntax__cev_fn(q, nd, "splice", TI_FN_SPLICE);
        syntax__nev_fn(q, nd, "set", TI_FN_SET);
        syntax__nev_fn(q, nd, "startswith", TI_FN_STARTSWITH);
        syntax__nev_fn(q, nd, "str", TI_FN_STR);
        syntax__nev_fn(q, nd, "syntax_err", TI_FN_SYNTAX_ERR);
        syntax__tev_fn(q, nd, "set_password", TI_FN_SET_PASSWORD);
        syntax__tev_fn(q, nd, "set_quota", TI_FN_SET_QUOTA);
        syntax__zev_fn(q, nd, "set_log_level", TI_FN_SET_LOG_LEVEL);
        syntax__zev_fn(q, nd, "shutdown", TI_FN_SHUTDOWN);
        break;
    case 't':
        syntax__nev_fn(q, nd, "t", TI_FN_T);
        syntax__nev_fn(q, nd, "test", TI_FN_TEST);
        syntax__nev_fn(q, nd, "try", TI_FN_TRY);
        syntax__nev_fn(q, nd, "type", TI_FN_TYPE);
        break;
    case 'u':
        syntax__nev_fn(q, nd, "upper", TI_FN_UPPER);
        syntax__zev_fn(q, nd, "user_info", TI_FN_USER_INFO);
        syntax__zev_fn(q, nd, "users_info", TI_FN_USERS_INFO);
        break;
    case 'v':
        syntax__nev_fn(q, nd, "values", TI_FN_VALUES);
        break;
    case 'w':
        syntax__bev_fn(q, nd, "wse", TI_FN_WSE);
        break;
    case 'x':
    case 'y':
        break;
    case 'z':
        syntax__nev_fn(q, nd, "zero_div_err", TI_FN_ZERO_DIV_ERR);
        break;
    }

    nd->data = (void *) ((uintptr_t) TI_FN_0);  /* unknown function */
}

static inline void syntax__investigate(ti_syntax_t * syntax, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE ||
            nd->cl_obj->gid == CLERI_GID_CHAIN);

    /* skip . and !, goto choice */
    ti_syntax_investigate(syntax, nd->children->next->node);

    /* index */
    if (nd->children->next->next->node->children)
        ti_syntax_investigate(syntax, nd->children->next->next->node);

    /* chain */
    if (nd->children->next->next->next)
        syntax__investigate(syntax, nd->children->next->next->next->node);
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

    assert(nd->cl_obj->tp == CLERI_TP_PRIO);

    node = nd->children->node;

    if (node->cl_obj->gid == CLERI_GID_SCOPE)
    {
        syntax__investigate(syntax, node);
        return false;
    }

    assert (node->cl_obj->tp == CLERI_TP_SEQUENCE);

    gid = node->children->next->node->cl_obj->gid;

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

/*
 * Investigates the following:
 *
 * - array sizes (stored to node->data)
 * - function bindings (stored to node->data)
 * - event requirement (set as syntax flags)
 * - closure detection and event requirement (set as flag to node->data)
 * - re-arrange operations to honor compare order
 * - count primitives cache requirements
 */
void ti_syntax_investigate(ti_syntax_t * syntax, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
    {
        uintptr_t sz = 0;
        cleri_children_t * child = nd          /* sequence */
                ->children->next->node         /* list */
                ->children;
        for (; child; child = child->next->next)
        {
            syntax__investigate(syntax, child->node);  /* scope */
            ++sz;

            if (!child->next)
                break;
        }
        nd->data = (void *) sz;
        return;
    }
    case CLERI_GID_BLOCK:
    {
        cleri_children_t * child = nd           /* seq<{, comment, list, }> */
                ->children->next->next->node    /* list statements */
                ->children;                     /* first child, not empty */
        while (1)
        {
            assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);
            syntax__investigate(syntax, child->node);  /* scope */
            if (!child->next || !(child = child->next->next))
                break;
        }
        return;
    }
    case CLERI_GID_CHAIN:
    case CLERI_GID_SCOPE:
        syntax__investigate(syntax, nd);        /* scope/chain */
        return;
    case CLERI_GID_THING:
    {
        cleri_children_t * child = nd           /* sequence */
                ->children->next->node          /* list */
                ->children;
        for (; child; child = child->next->next)
        {
            /* sequence(name: scope) (only investigate the scopes */
            syntax__investigate(
                    syntax,
                    child->node->children->next->next->node);  /* scope */

            if (!child->next)
                break;
        }
        return;
    }
    case CLERI_GID_FUNCTION:
    {
        cleri_children_t * child = nd          /* sequence */
                ->children->next->next->node   /* list (arguments) */
                ->children;

        /* map function to node */
        syntax__map_fn(syntax, nd->children->node);

        /* investigate arguments */
        for (; child; child = child->next->next)
        {
            syntax__investigate(syntax, child->node);  /* scope */
            if (!child->next)
                break;
        }
        return;
    }
    case CLERI_GID_ASSIGNMENT:
        syntax__set_collection_event(syntax);
        /* fall through */
    case CLERI_GID_VAR_ASSIGN:
        /* investigate the scope */
        syntax__investigate(syntax, nd->children->next->next->node);
        return;
    case CLERI_GID_STATEMENTS:
    {
        cleri_children_t * child = nd->children;
        while(child)
        {
            syntax__investigate(syntax, child->node);  /* scope */
            if (!child->next)
                break;
            child = child->next->next;                  /* skip delimiter */
        }
        return;
    }
    case CLERI_GID_INDEX:
        for (   cleri_children_t * child = nd->children;
                child;
                child = child->next)
            /* sequence('[', scope, ']') (only investigate the scopes */
            syntax__investigate(
                    syntax,
                    child->node->children->next->node);  /* scope */
        return;
    case CLERI_GID_VAR:
    case CLERI_GID_NAME:
        return;
    case CLERI_GID_O_NOT:
    case CLERI_GID_COMMENT:
        assert (0);  /* not's and comments are already filtered */
        return;
    case CLERI_GID_IMMUTABLE:
        nd = nd->children->node;
        switch (nd->cl_obj->gid)
        {
        case CLERI_GID_T_CLOSURE:
            /* investigate the scope, the rest can be skipped */
            syntax__investigate(
                    syntax,
                    nd->children->next->next->next->node);
            /* fall through */
        case CLERI_GID_T_INT:
        case CLERI_GID_T_FLOAT:
        case CLERI_GID_T_STRING:
        case CLERI_GID_T_REGEX:
            ++syntax->val_cache_n;
            nd->data = NULL;        /* initialize data to null */
        }
        return;
    case CLERI_GID_OPERATIONS:
        (void) syntax__swap_opr(syntax, nd->children->next, 0);
        if (nd->children->next->next->next)     /* optional ? (sequence) */
        {
            cleri_children_t * child = \
                    nd->children->next->next->next->node->children->next;
            syntax__investigate(syntax, child->node);
            if (child->next)  /* else : case */
                syntax__investigate(
                        syntax,
                        child->next->node->children->next->node);
        }
        return;
    }

    assert (0);  /* Everything should be handled */

    for (cleri_children_t * child = nd->children; child; child = child->next)
        ti_syntax_investigate(syntax, child->node);
}

void ti_syntax_init(ti_syntax_t * syntax, uint8_t flags)
{
    syntax->val_cache_n = 0;
    syntax->flags = flags;
    /*
     * Properties `deep` and `pkg_id` are only used by `ti_query_t` and
     * are not initialized here.
     */
}
