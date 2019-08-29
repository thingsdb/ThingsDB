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

#include <ti/fn/fnadd.h>
#include <ti/fn/fnarray.h>
#include <ti/fn/fnassert.h>
#include <ti/fn/fnblob.h>
#include <ti/fn/fnbool.h>
#include <ti/fn/fncall.h>
#include <ti/fn/fncollectioninfo.h>
#include <ti/fn/fncollectionsinfo.h>
#include <ti/fn/fncontains.h>
#include <ti/fn/fncounters.h>
#include <ti/fn/fndeep.h>
#include <ti/fn/fndel.h>
#include <ti/fn/fndelcollection.h>
#include <ti/fn/fndelexpired.h>
#include <ti/fn/fndelprocedure.h>
#include <ti/fn/fndeltoken.h>
#include <ti/fn/fndeluser.h>
#include <ti/fn/fnendswith.h>
#include <ti/fn/fnerr.h>
#include <ti/fn/fnerrors.h>
#include <ti/fn/fnextend.h>
#include <ti/fn/fnfilter.h>
#include <ti/fn/fnfind.h>
#include <ti/fn/fnfindindex.h>
#include <ti/fn/fnfloat.h>
#include <ti/fn/fnget.h>
#include <ti/fn/fngrant.h>
#include <ti/fn/fnhas.h>
#include <ti/fn/fnid.h>
#include <ti/fn/fnindexof.h>
#include <ti/fn/fnint.h>
#include <ti/fn/fnisarray.h>
#include <ti/fn/fnisascii.h>
#include <ti/fn/fnisbool.h>
#include <ti/fn/fniserr.h>
#include <ti/fn/fnisfloat.h>
#include <ti/fn/fnisinf.h>
#include <ti/fn/fnisint.h>
#include <ti/fn/fnislist.h>
#include <ti/fn/fnisnan.h>
#include <ti/fn/fnisnil.h>
#include <ti/fn/fnisraw.h>
#include <ti/fn/fnisset.h>
#include <ti/fn/fnisthing.h>
#include <ti/fn/fnistuple.h>
#include <ti/fn/fnisutf8.h>
#include <ti/fn/fnkeys.h>
#include <ti/fn/fnlen.h>
#include <ti/fn/fnlower.h>
#include <ti/fn/fnmap.h>
#include <ti/fn/fnnewcollection.h>
#include <ti/fn/fnnewnode.h>
#include <ti/fn/fnnewprocedure.h>
#include <ti/fn/fnnewtoken.h>
#include <ti/fn/fnnewuser.h>
#include <ti/fn/fnnodeinfo.h>
#include <ti/fn/fnnodesinfo.h>
#include <ti/fn/fnnow.h>
#include <ti/fn/fnpop.h>
#include <ti/fn/fnpopnode.h>
#include <ti/fn/fnproceduredoc.h>
#include <ti/fn/fnprocedureinfo.h>
#include <ti/fn/fnproceduresinfo.h>
#include <ti/fn/fnpush.h>
#include <ti/fn/fnraise.h>
#include <ti/fn/fnrefs.h>
#include <ti/fn/fnremove.h>
#include <ti/fn/fnrenamecollection.h>
#include <ti/fn/fnrenameuser.h>
#include <ti/fn/fnreplacenode.h>
#include <ti/fn/fnresetcounters.h>
#include <ti/fn/fnreturn.h>
#include <ti/fn/fnrevoke.h>
#include <ti/fn/fnrun.h>
#include <ti/fn/fnset.h>
#include <ti/fn/fnsetloglevel.h>
#include <ti/fn/fnsetpassword.h>
#include <ti/fn/fnsetquota.h>
#include <ti/fn/fnshutdown.h>
#include <ti/fn/fnsplice.h>
#include <ti/fn/fnstartswith.h>
#include <ti/fn/fnstr.h>
#include <ti/fn/fnt.h>
#include <ti/fn/fntest.h>
#include <ti/fn/fntry.h>
#include <ti/fn/fntype.h>
#include <ti/fn/fnupper.h>
#include <ti/fn/fnuserinfo.h>
#include <ti/fn/fnusersinfo.h>
#include <ti/fn/fnvalues.h>
#include <ti/fn/fnwse.h>


#define SYNTAX__X(__ev, __q, __nd, __str, __fn)                             \
do if (__nd->len == strlen(__str) && !memcmp(__nd->str, __str, __nd->len))  \
{                                                                           \
    __nd->data = &__fn;                                                     \
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

static void syntax__map_fn(ti_syntax_t * q, cleri_node_t * nd, _Bool chain)
{
    /* a function name has at least size 1 */
    switch ((ti_alpha_lower_t) *nd->str)
    {
    case 'a':
        syntax__cev_fn(q, nd, "add", do__f_add);
        syntax__nev_fn(q, nd, "array", do__f_array);
        syntax__nev_fn(q, nd, "assert", do__f_assert);
        syntax__nev_fn(q, nd, "assert_err", do__f_assert_err);
        syntax__nev_fn(q, nd, "auth_err", do__f_auth_err);
        break;
    case 'b':
        syntax__nev_fn(q, nd, "bad_data_err", do__f_bad_data_err);
        syntax__nev_fn(q, nd, "blob", do__f_blob);
        syntax__nev_fn(q, nd, "bool", do__f_bool);
        break;
    case 'c':
        syntax__nev_fn(q, nd, "call", do__f_call);
        syntax__nev_fn(q, nd, "contains", do__f_contains);
        syntax__zev_fn(q, nd, "collection_info", do__f_collection_info);
        syntax__zev_fn(q, nd, "collections_info", do__f_collections_info);
        syntax__zev_fn(q, nd, "counters", do__f_counters);
        break;
    case 'd':
        syntax__cev_fn(q, nd, "del", do__f_del);  /* most frequent used */
        syntax__bev_fn(q, nd, "del_procedure", do__f_del_procedure);
        syntax__nev_fn(q, nd, "deep", do__f_deep);
        syntax__tev_fn(q, nd, "del_collection", do__f_del_collection);
        syntax__tev_fn(q, nd, "del_expired", do__f_del_expired);
        syntax__tev_fn(q, nd, "del_token", do__f_del_token);
        syntax__tev_fn(q, nd, "del_user", do__f_del_user);
        break;
    case 'e':
        syntax__cev_fn(q, nd, "extend", do__f_extend);
        syntax__nev_fn(q, nd, "endswith", do__f_endswith);
        syntax__nev_fn(q, nd, "err", do__f_err);
        break;
    case 'f':
        syntax__nev_fn(q, nd, "filter", do__f_filter);
        syntax__nev_fn(q, nd, "find", do__f_find);
        syntax__nev_fn(q, nd, "findindex", do__f_findindex);
        syntax__nev_fn(q, nd, "float", do__f_float);
        syntax__nev_fn(q, nd, "forbidden_err", do__f_forbidden_err);
        break;
    case 'g':
        syntax__nev_fn(q, nd, "get", do__f_get);
        syntax__tev_fn(q, nd, "grant", do__f_grant);
        break;
    case 'h':
        syntax__nev_fn(q, nd, "has", do__f_has);
        break;
    case 'i':
        syntax__nev_fn(q, nd, "id", do__f_id);
        syntax__nev_fn(q, nd, "index_err", do__f_index_err);
        syntax__nev_fn(q, nd, "indexof", do__f_indexof);
        syntax__nev_fn(q, nd, "int", do__f_int);
        syntax__nev_fn(q, nd, "isarray", do__f_isarray);
        syntax__nev_fn(q, nd, "isascii", do__f_isascii);
        syntax__nev_fn(q, nd, "isbool", do__f_isbool);
        syntax__nev_fn(q, nd, "iserr", do__f_iserr);
        syntax__nev_fn(q, nd, "isfloat", do__f_isfloat);
        syntax__nev_fn(q, nd, "isinf", do__f_isinf);
        syntax__nev_fn(q, nd, "isint", do__f_isint);
        syntax__nev_fn(q, nd, "islist", do__f_islist);
        syntax__nev_fn(q, nd, "isnan", do__f_isnan);
        syntax__nev_fn(q, nd, "isnil", do__f_isnil);
        syntax__nev_fn(q, nd, "israw", do__f_israw);
        syntax__nev_fn(q, nd, "isset", do__f_isset);
        syntax__nev_fn(q, nd, "isstr", do__f_isstr);
        syntax__nev_fn(q, nd, "isthing", do__f_isthing);
        syntax__nev_fn(q, nd, "istuple", do__f_istuple);
        syntax__nev_fn(q, nd, "isutf8", do__f_isutf8);
        break;
    case 'j':
        break;
    case 'k':
        syntax__nev_fn(q, nd, "keys", do__f_keys);
        break;
    case 'l':
        syntax__nev_fn(q, nd, "len", do__f_len);
        syntax__nev_fn(q, nd, "lower", do__f_lower);
        break;
    case 'm':
        syntax__nev_fn(q, nd, "map", do__f_map);
        syntax__nev_fn(q, nd, "max_quota_err", do__f_max_quota_err);
        break;
    case 'n':
        syntax__bev_fn(q, nd, "new_procedure", do__f_new_procedure);
        syntax__nev_fn(q, nd, "node_err", do__f_node_err);
        syntax__nev_fn(q, nd, "now", do__f_now);
        syntax__tev_fn(q, nd, "new_collection", do__f_new_collection);
        syntax__tev_fn(q, nd, "new_node", do__f_new_node);
        syntax__tev_fn(q, nd, "new_token", do__f_new_token);
        syntax__tev_fn(q, nd, "new_user", do__f_new_user);
        syntax__zev_fn(q, nd, "node_info", do__f_node_info);
        syntax__zev_fn(q, nd, "nodes_info", do__f_nodes_info);
        break;
    case 'o':
        syntax__nev_fn(q, nd, "overflow_err", do__f_overflow_err);
        break;
    case 'p':
        syntax__cev_fn(q, nd, "pop", do__f_pop);
        syntax__cev_fn(q, nd, "push", do__f_push);
        syntax__nev_fn(q, nd, "procedure_doc", do__f_procedure_doc);
        syntax__nev_fn(q, nd, "procedure_info", do__f_procedure_info);
        syntax__nev_fn(q, nd, "procedures_info", do__f_procedures_info);
        syntax__tev_fn(q, nd, "pop_node", do__f_pop_node);
        break;
    case 'q':
        break;
    case 'r':
        syntax__cev_fn(q, nd, "remove", do__f_remove);
        syntax__nev_fn(q, nd, "raise", do__f_raise);
        syntax__nev_fn(q, nd, "refs", do__f_refs);
        syntax__nev_fn(q, nd, "return", do__f_return);
        syntax__nev_fn(q, nd, "run", do__f_run);
        syntax__tev_fn(q, nd, "rename_collection", do__f_rename_collection);
        syntax__tev_fn(q, nd, "rename_user", do__f_rename_user);
        syntax__tev_fn(q, nd, "replace_node", do__f_replace_node);
        syntax__tev_fn(q, nd, "revoke", do__f_revoke);
        syntax__zev_fn(q, nd, "reset_counters", do__f_reset_counters);
        break;
    case 's':
        syntax__cev_fn(q, nd, "splice", do__f_splice);
        if (chain)
            syntax__cev_fn(q, nd, "set", do__f_set);
        else
            syntax__nev_fn(q, nd, "set", do__f_set);
        syntax__nev_fn(q, nd, "startswith", do__f_startswith);
        syntax__nev_fn(q, nd, "str", do__f_str);
        syntax__nev_fn(q, nd, "syntax_err", do__f_syntax_err);
        syntax__tev_fn(q, nd, "set_password", do__f_set_password);
        syntax__tev_fn(q, nd, "set_quota", do__f_set_quota);
        syntax__zev_fn(q, nd, "set_log_level", do__f_set_log_level);
        syntax__zev_fn(q, nd, "shutdown", do__f_shutdown);
        break;
    case 't':
        syntax__nev_fn(q, nd, "t", do__f_t);
        syntax__nev_fn(q, nd, "test", do__f_test);
        syntax__nev_fn(q, nd, "try", do__f_try);
        syntax__nev_fn(q, nd, "type", do__f_type);
        break;
    case 'u':
        syntax__nev_fn(q, nd, "upper", do__f_upper);
        syntax__zev_fn(q, nd, "user_info", do__f_user_info);
        syntax__zev_fn(q, nd, "users_info", do__f_users_info);
        break;
    case 'v':
        syntax__nev_fn(q, nd, "values", do__f_values);
        break;
    case 'w':
        syntax__bev_fn(q, nd, "wse", do__f_wse);
        break;
    case 'x':
    case 'y':
        break;
    case 'z':
        syntax__nev_fn(q, nd, "zero_div_err", do__f_zero_div_err);
        break;
    }

    nd->data = NULL;  /* unknown function */
}

static inline void syntax__investigate(ti_syntax_t * syntax, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE ||
            nd->cl_obj->gid == CLERI_GID_CHAIN);

    /* skip . and !, goto choice */
    ti_syntax_inv(
            syntax,
            nd->children->next->node,
            nd->cl_obj->gid == CLERI_GID_CHAIN);

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
 * - array sizes (stored in node->data)
 * - number of function arguments (stored in node->data)
 * - function bindings (stored in node->data)
 * - event requirement (set as syntax flags)
 * - closure detection and event requirement (set as flag to node->data)
 * - re-arrange operations to honor compare order
 * - count primitives cache requirements
 */
void ti_syntax_inv(ti_syntax_t * syntax, cleri_node_t * nd, _Bool chain)
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
        for (; child; child = child->next->next)
        {
            syntax__investigate(syntax, child->node);  /* scope */

            if (!child->next)
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
        intptr_t nargs = 0;
        cleri_children_t * child;

        /* map function to node */
        syntax__map_fn(syntax, nd->children->node, chain);

        /* list (arguments) */
        nd = nd->children->next->next->node;

        /* investigate arguments */
        for (child = nd->children; child; child = child->next->next)
        {
            syntax__investigate(syntax, child->node);  /* scope */
            ++nargs;

            if (!child->next)
                break;
        }

        /* bind `nargs` to node->data */
        nd->data = (void *) nargs;
        return;
    }
    case CLERI_GID_ASSIGNMENT:
        syntax__set_collection_event(syntax);
        /* fall through */
    case CLERI_GID_VAR_ASSIGN:
        /* investigate the scope */
        syntax__investigate(syntax, nd->children->next->next->node);
        nd->children->node->data = NULL;
        ++syntax->val_cache_n;
        return;
    case CLERI_GID_STATEMENTS:
    {
        cleri_children_t * child = nd->children;
        for (; child; child = child->next->next)
        {
            syntax__investigate(syntax, child->node);   /* scope */

            if (!child->next)
                break;
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
        nd->data = NULL;
        ++syntax->val_cache_n;
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
