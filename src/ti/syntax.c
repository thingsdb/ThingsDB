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
#include <ti/fn/fnbackupinfo.h>
#include <ti/fn/fnbackupsinfo.h>
#include <ti/fn/fnbool.h>
#include <ti/fn/fnbytes.h>
#include <ti/fn/fncall.h>
#include <ti/fn/fncollectioninfo.h>
#include <ti/fn/fncollectionsinfo.h>
#include <ti/fn/fncontains.h>
#include <ti/fn/fncounters.h>
#include <ti/fn/fndeep.h>
#include <ti/fn/fndel.h>
#include <ti/fn/fndelbackup.h>
#include <ti/fn/fndelcollection.h>
#include <ti/fn/fndelexpired.h>
#include <ti/fn/fndelnode.h>
#include <ti/fn/fndelprocedure.h>
#include <ti/fn/fndeltoken.h>
#include <ti/fn/fndeltype.h>
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
#include <ti/fn/fnisbytes.h>
#include <ti/fn/fniserr.h>
#include <ti/fn/fnisfloat.h>
#include <ti/fn/fnisinf.h>
#include <ti/fn/fnisint.h>
#include <ti/fn/fnislist.h>
#include <ti/fn/fnisnan.h>
#include <ti/fn/fnisnil.h>
#include <ti/fn/fnisraw.h>
#include <ti/fn/fnisset.h>
#include <ti/fn/fnisstr.h>
#include <ti/fn/fnisthing.h>
#include <ti/fn/fnistuple.h>
#include <ti/fn/fnisutf8.h>
#include <ti/fn/fnkeys.h>
#include <ti/fn/fnlen.h>
#include <ti/fn/fnlower.h>
#include <ti/fn/fnmap.h>
#include <ti/fn/fnmodtype.h>
#include <ti/fn/fnnew.h>
#include <ti/fn/fnnewbackup.h>
#include <ti/fn/fnnewcollection.h>
#include <ti/fn/fnnewnode.h>
#include <ti/fn/fnnewprocedure.h>
#include <ti/fn/fnnewtoken.h>
#include <ti/fn/fnnewtype.h>
#include <ti/fn/fnnewuser.h>
#include <ti/fn/fnnodeinfo.h>
#include <ti/fn/fnnodesinfo.h>
#include <ti/fn/fnnow.h>
#include <ti/fn/fnpop.h>
#include <ti/fn/fnproceduredoc.h>
#include <ti/fn/fnprocedureinfo.h>
#include <ti/fn/fnproceduresinfo.h>
#include <ti/fn/fnpush.h>
#include <ti/fn/fnraise.h>
#include <ti/fn/fnrefs.h>
#include <ti/fn/fnremove.h>
#include <ti/fn/fnrenamecollection.h>
#include <ti/fn/fnrenameuser.h>
#include <ti/fn/fnresetcounters.h>
#include <ti/fn/fnreturn.h>
#include <ti/fn/fnrevoke.h>
#include <ti/fn/fnrun.h>
#include <ti/fn/fnset.h>
#include <ti/fn/fnsetloglevel.h>
#include <ti/fn/fnsetpassword.h>
#include <ti/fn/fnsetquota.h>
#include <ti/fn/fnsettype.h>
#include <ti/fn/fnshutdown.h>
#include <ti/fn/fnsort.h>
#include <ti/fn/fnsplice.h>
#include <ti/fn/fnstartswith.h>
#include <ti/fn/fnstr.h>
#include <ti/fn/fntest.h>
#include <ti/fn/fnthing.h>
#include <ti/fn/fntry.h>
#include <ti/fn/fntype.h>
#include <ti/fn/fntypecount.h>
#include <ti/fn/fntypeinfo.h>
#include <ti/fn/fntypesinfo.h>
#include <ti/fn/fnunwrap.h>
#include <ti/fn/fnupper.h>
#include <ti/fn/fnuserinfo.h>
#include <ti/fn/fnusersinfo.h>
#include <ti/fn/fnvalues.h>
#include <ti/fn/fnwrap.h>
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
    syntax->flags |= syntax->flags & (
                TI_SYNTAX_FLAG_THINGSDB|TI_SYNTAX_FLAG_COLLECTION
            ) ? TI_SYNTAX_FLAG_EVENT : 0;
}

static void syntax__statement(ti_syntax_t * syntax, cleri_node_t * nd);

enum
{
    FN__CHAIN   = 1<<0,
    FN__ON_VAR  = TI_SYNTAX_FLAG_ON_VAR,
};

static void syntax__map_fn(ti_syntax_t * q, cleri_node_t * nd, int flags)
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
        syntax__nev_fn(q, nd, "bool", do__f_bool);
        syntax__nev_fn(q, nd, "bytes", do__f_bytes);
        syntax__nev_fn(q, nd, "bad_data_err", do__f_bad_data_err);
        syntax__zev_fn(q, nd, "backup_info", do__f_backup_info);
        syntax__zev_fn(q, nd, "backups_info", do__f_backups_info);
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
        syntax__nev_fn(q, nd, "deep", do__f_deep);
        syntax__tev_fn(q, nd, "del_expired", do__f_del_expired);
        syntax__tev_fn(q, nd, "del_token", do__f_del_token);
        syntax__bev_fn(q, nd, "del_procedure", do__f_del_procedure);
        syntax__cev_fn(q, nd, "del_type", do__f_del_type);
        syntax__tev_fn(q, nd, "del_collection", do__f_del_collection);
        syntax__tev_fn(q, nd, "del_user", do__f_del_user);
        syntax__tev_fn(q, nd, "del_node", do__f_del_node);
        syntax__zev_fn(q, nd, "del_backup", do__f_del_backup);
        break;
    case 'e':
        if (flags & FN__ON_VAR)
            syntax__nev_fn(q, nd, "extend", do__f_extend);
        else
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
        syntax__nev_fn(q, nd, "indexof", do__f_indexof);
        syntax__nev_fn(q, nd, "int", do__f_int);
        syntax__nev_fn(q, nd, "isarray", do__f_isarray);
        syntax__nev_fn(q, nd, "isascii", do__f_isascii);
        syntax__nev_fn(q, nd, "isbool", do__f_isbool);
        syntax__nev_fn(q, nd, "isbytes", do__f_isbytes);
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
        syntax__nev_fn(q, nd, "lookup_err", do__f_lookup_err);
        break;
    case 'm':
        syntax__nev_fn(q, nd, "map", do__f_map);  /* most frequent used */
        syntax__cev_fn(q, nd, "mod_type", do__f_mod_type);
        syntax__nev_fn(q, nd, "max_quota_err", do__f_max_quota_err);
        break;
    case 'n':
        syntax__nev_fn(q, nd, "new", do__f_new);  /* most frequent used */
        syntax__bev_fn(q, nd, "new_procedure", do__f_new_procedure);
        syntax__cev_fn(q, nd, "new_type", do__f_new_type);
        syntax__nev_fn(q, nd, "node_err", do__f_node_err);
        syntax__nev_fn(q, nd, "now", do__f_now);
        syntax__nev_fn(q, nd, "num_arguments_err", do__f_num_arguments_err);
        syntax__tev_fn(q, nd, "new_collection", do__f_new_collection);
        syntax__tev_fn(q, nd, "new_node", do__f_new_node);
        syntax__tev_fn(q, nd, "new_token", do__f_new_token);
        syntax__tev_fn(q, nd, "new_user", do__f_new_user);
        syntax__zev_fn(q, nd, "new_backup", do__f_new_backup);
        syntax__zev_fn(q, nd, "node_info", do__f_node_info);
        syntax__zev_fn(q, nd, "nodes_info", do__f_nodes_info);
        break;
    case 'o':
        syntax__nev_fn(q, nd, "overflow_err", do__f_overflow_err);
        syntax__nev_fn(q, nd, "operation_err", do__f_operation_err);
        break;
    case 'p':
        if (flags & FN__ON_VAR)
        {
            syntax__nev_fn(q, nd, "pop", do__f_pop);
            syntax__nev_fn(q, nd, "push", do__f_push);
        }
        else
        {
            syntax__cev_fn(q, nd, "pop", do__f_pop);
            syntax__cev_fn(q, nd, "push", do__f_push);
        }
        syntax__nev_fn(q, nd, "procedure_doc", do__f_procedure_doc);
        syntax__nev_fn(q, nd, "procedure_info", do__f_procedure_info);
        syntax__nev_fn(q, nd, "procedures_info", do__f_procedures_info);
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
        syntax__tev_fn(q, nd, "revoke", do__f_revoke);
        syntax__zev_fn(q, nd, "reset_counters", do__f_reset_counters);
        break;
    case 's':
        if (flags & FN__ON_VAR)
            syntax__nev_fn(q, nd, "splice", do__f_splice);
        else
            syntax__cev_fn(q, nd, "splice", do__f_splice);
        if (flags & FN__CHAIN)
            syntax__cev_fn(q, nd, "set", do__f_set);
        else
            syntax__nev_fn(q, nd, "set", do__f_set);
        syntax__nev_fn(q, nd, "startswith", do__f_startswith);
        syntax__nev_fn(q, nd, "str", do__f_str);
        syntax__nev_fn(q, nd, "sort", do__f_sort);
        syntax__nev_fn(q, nd, "syntax_err", do__f_syntax_err);
        syntax__cev_fn(q, nd, "set_type", do__f_set_type);
        syntax__tev_fn(q, nd, "set_password", do__f_set_password);
        syntax__tev_fn(q, nd, "set_quota", do__f_set_quota);
        syntax__zev_fn(q, nd, "set_log_level", do__f_set_log_level);
        syntax__zev_fn(q, nd, "shutdown", do__f_shutdown);
        break;
    case 't':
        syntax__nev_fn(q, nd, "thing", do__f_thing);  /* most frequent used */
        syntax__nev_fn(q, nd, "test", do__f_test);
        syntax__nev_fn(q, nd, "try", do__f_try);
        syntax__nev_fn(q, nd, "type", do__f_type);
        syntax__nev_fn(q, nd, "type_count", do__f_type_count);
        syntax__nev_fn(q, nd, "type_err", do__f_type_err);
        syntax__nev_fn(q, nd, "type_info", do__f_type_info);
        syntax__nev_fn(q, nd, "types_info", do__f_types_info);
        break;
    case 'u':
        syntax__nev_fn(q, nd, "unwrap", do__f_unwrap);  /* most frequent used */
        syntax__nev_fn(q, nd, "upper", do__f_upper);
        syntax__zev_fn(q, nd, "user_info", do__f_user_info);
        syntax__zev_fn(q, nd, "users_info", do__f_users_info);
        break;
    case 'v':
        syntax__nev_fn(q, nd, "values", do__f_values);
        syntax__nev_fn(q, nd, "value_err", do__f_value_err);
        break;
    case 'w':
        syntax__bev_fn(q, nd, "wse", do__f_wse);
        syntax__nev_fn(q, nd, "wrap", do__f_wrap);
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

static _Bool syntax__operations(
        ti_syntax_t * syntax,
        cleri_children_t * parent,
        uint32_t parent_gid)
{
    uint32_t gid = parent->node->children->next->node->cl_obj->gid;
    cleri_children_t * childb = parent->node->children->next->next;

    assert (gid >= CLERI_GID_OPR0_MUL_DIV_MOD &&
            gid <= CLERI_GID_OPR8_TERNARY);

    syntax__statement(syntax, parent->node->children->node);

    if (gid == CLERI_GID_OPR8_TERNARY)
        syntax__statement(
                syntax,
                parent->node->children->next->node->children->next->node);

    if (childb->node->children->node->cl_obj->gid != CLERI_GID_OPERATIONS)
    {
        syntax__statement(syntax, childb->node);
    }
    else if (syntax__operations(syntax, childb->node->children, gid))
    {
        /* Swap operations */
        cleri_children_t * syntax_childa;
        cleri_node_t * tmp = parent->node;  /* operations */

        parent->node = childb->node->children->node;  /* operations */

        gid = parent->node->children->next->node->cl_obj->gid;

        assert (gid >= CLERI_GID_OPR0_MUL_DIV_MOD &&
                gid <= CLERI_GID_OPR8_TERNARY);

        syntax_childa = parent->node->children->node->children;
        childb->node->children->node = syntax_childa->node;
        syntax_childa->node = tmp;
    }

    return gid > parent_gid;
}

static void syntax__function(ti_syntax_t * syntax, cleri_node_t * nd, int flags)
{
    intptr_t nargs = 0;
    cleri_children_t * child;

    /* map function to node */
    syntax__map_fn(syntax, nd->children->node, flags);

    /* list (arguments) */
    nd = nd->children->next->node->children->next->node;

    /* investigate arguments */
    for (child = nd->children; child; child = child->next->next)
    {
        syntax__statement(syntax, child->node);  /* statement */
        ++nargs;

        if (!child->next)
            break;
    }

    /* bind `nargs` to node->data */
    nd->data = (void *) nargs;
}

static void syntax__index(ti_syntax_t * syntax, cleri_node_t * nd)
{
    cleri_children_t * child = nd->children;
    assert (child);
    do
    {
        cleri_children_t * c = child->node     /* sequence */
                ->children->next->node         /* slice */
                ->children;

        if (child->node->children->next->next->next)
        {
            syntax__set_collection_event(syntax);
            syntax__statement(syntax, child->node           /* sequence */
                    ->children->next->next->next->node      /* assignment */
                    ->children->next->node);                /* statement */
        }

        for (; c; c = c->next)
            if (c->node->cl_obj->gid == CLERI_GID_STATEMENT)
                syntax__statement(syntax, c->node);
    }
    while ((child = child->next));
}

static inline void syntax__thing(ti_syntax_t * syntax, cleri_node_t * nd)
{
    uintptr_t sz = 0;
    cleri_children_t * child = nd           /* sequence */
            ->children->next->node          /* list */
            ->children;
    for (; child; child = child->next->next)
    {
        /* sequence(name: statement) (only investigate the statements */
        syntax__statement(
                syntax,
                child->node->children->next->next->node);  /* statement */
        ++sz;
        if (!child->next)
            break;
    }
    nd->data = (void *) sz;
}


static void syntax__var_opt_fa(ti_syntax_t * syntax, cleri_node_t * nd)
{
    if (nd->children->next)
    {
        switch (nd->children->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            syntax__function(syntax, nd, 0);
            return;
        case CLERI_GID_ASSIGN:
            syntax__statement(
                    syntax,
                    nd->children->next->node->children->next->node);
            break;
        case CLERI_GID_INSTANCE:
            syntax__thing(syntax, nd->children->next->node);
            return;
        default:
            assert (0);
            return;
        }
    }
    else
        syntax->flags |= TI_SYNTAX_FLAG_ON_VAR;

    nd->children->node->data = NULL;
    ++syntax->val_cache_n;
}

static void syntax__name_opt_fa(ti_syntax_t * syntax, cleri_node_t * nd)
{
    if (nd->children->next)
    {
        switch (nd->children->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            syntax__function(
                    syntax,
                    nd,
                    FN__CHAIN | (syntax->flags & TI_SYNTAX_FLAG_ON_VAR));
            return;
        case CLERI_GID_ASSIGN:
            syntax__set_collection_event(syntax);
            syntax__statement(
                    syntax,
                    nd->children->next->node->children->next->node);
            break;
        default:
            assert (0);
            return;
        }
    }
    nd->children->node->data = NULL;
    ++syntax->val_cache_n;
}

static inline void syntax__chain(ti_syntax_t * syntax, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);

    syntax__name_opt_fa(syntax, nd->children->next->node);

    syntax->flags &= ~TI_SYNTAX_FLAG_ON_VAR;

    /* index */
    if (nd->children->next->next->node->children)
        syntax__index(syntax, nd->children->next->next->node);

    /* chain */
    if (nd->children->next->next->next)
        syntax__chain(syntax, nd->children->next->next->next->node);
}

static void syntax__expr_choice(ti_syntax_t * syntax, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        syntax__chain(syntax, nd);        /* chain */
        return;
    case CLERI_GID_THING_BY_ID:
        nd->data = NULL;
        ++syntax->val_cache_n;
        return;
    case CLERI_GID_IMMUTABLE:
        nd = nd->children->node;
        switch (nd->cl_obj->gid)
        {
        case CLERI_GID_T_CLOSURE:
            /* investigate the statement, the rest can be skipped */
            syntax__statement(
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
    case CLERI_GID_VAR_OPT_MORE:
        syntax__var_opt_fa(syntax, nd);
        return;
    case CLERI_GID_THING:
        syntax__thing(syntax,nd);
        return;
    case CLERI_GID_ARRAY:
    {
        uintptr_t sz = 0;
        cleri_children_t * child = nd          /* sequence */
                ->children->next->node         /* list */
                ->children;
        for (; child; child = child->next->next)
        {
            syntax__statement(syntax, child->node);  /* statement */
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
        do
        {
            syntax__statement(syntax, child->node);  /* statement */
        }
        while (child->next && (child = child->next->next));
        return;
    }
    case CLERI_GID_PARENTHESIS:
        syntax__statement(syntax, nd->children->next->node);
        return;
    }
    assert (0);
}

static inline void syntax__expression(ti_syntax_t * syntax, cleri_node_t * nd)
{
    cleri_children_t * child;
    intptr_t nots = 0;
    assert (nd->cl_obj->gid == CLERI_GID_EXPRESSION);

    for (child = nd->children->node->children; child; child = child->next)
        ++nots;

    nd->children->node->data = (void *) nots;

    syntax__expr_choice(syntax, nd->children->next->node);

    /* index */
    if (nd->children->next->next->node->children)
        syntax__index(syntax, nd->children->next->next->node);

    /* chain */
    if (nd->children->next->next->next)
        syntax__chain(syntax, nd->children->next->next->next->node);
}

static void syntax__statement(ti_syntax_t * syntax, cleri_node_t * nd)
{
    cleri_node_t * node;
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT);

    node = nd->children->node;
    switch (node->cl_obj->gid)
    {
    case CLERI_GID_EXPRESSION:
        syntax->flags &= ~TI_SYNTAX_FLAG_ON_VAR;
        syntax__expression(syntax, node);
        return;
    case CLERI_GID_OPERATIONS:
        syntax__operations(syntax, nd->children, 0);
        return;
    }
    assert (0);
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
void ti_syntax_probe(ti_syntax_t * syntax, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT ||
            nd->cl_obj->gid == CLERI_GID_STATEMENTS);

    if (nd->cl_obj->gid == CLERI_GID_STATEMENTS)
    {
        cleri_children_t * child = nd->children;

        for (; child; child = child->next->next)
        {
            syntax__statement(syntax, child->node);   /* statement */

            if (!child->next)
                return;
        }
        return;
    }
    return syntax__statement(syntax, nd);
}
