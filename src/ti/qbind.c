/*
 * ti/qbind.c
 */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <langdef/langdef.h>
#include <ti/val.h>
#include <tiinc.h>
#include <util/logger.h>

#include <ti/fn/fnadd.h>
#include <ti/fn/fnassert.h>
#include <ti/fn/fnbackupinfo.h>
#include <ti/fn/fnbackupsinfo.h>
#include <ti/fn/fnbase64decode.h>
#include <ti/fn/fnbase64encode.h>
#include <ti/fn/fnbool.h>
#include <ti/fn/fnbytes.h>
#include <ti/fn/fncall.h>
#include <ti/fn/fnchoice.h>
#include <ti/fn/fncode.h>
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
#include <ti/fn/fndoc.h>
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
#include <ti/fn/fnhasbackup.h>
#include <ti/fn/fnhascollection.h>
#include <ti/fn/fnhasnode.h>
#include <ti/fn/fnhasprocedure.h>
#include <ti/fn/fnhastoken.h>
#include <ti/fn/fnhastype.h>
#include <ti/fn/fnhasuser.h>
#include <ti/fn/fnid.h>
#include <ti/fn/fnindexof.h>
#include <ti/fn/fnif.h>
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
#include <ti/fn/fnlist.h>
#include <ti/fn/fnlower.h>
#include <ti/fn/fnmap.h>
#include <ti/fn/fnmodtype.h>
#include <ti/fn/fnmsg.h>
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
#include <ti/fn/fnrand.h>
#include <ti/fn/fnrandint.h>
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
#include <ti/qbind.h>


#define SYNTAX__X(__ev, __q, __nd, __str, __fn)                             \
do if (!memcmp(__nd->str, __str, __nd->len))                                \
{                                                                           \
    __nd->data = &__fn;                                                     \
    __ev(__q);                                                              \
    return;                                                                 \
} while(0)

/* set 'c'ollection event, used for collection scope */
#define qbind__cev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X(qbind__set_collection_event, __q, __nd, __str, __fn)

/* 'n'o event, used for collection scope */
#define qbind__nev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X((void), __q, __nd, __str, __fn)

#define qbind__vev_fn(__q, __nd, __str, __fn, __flags) \
    do { \
        if (__flags & FN__ON_VAR) { \
            SYNTAX__X((void), __q, __nd, __str, __fn); \
        } else { \
            SYNTAX__X(qbind__set_collection_event, __q, __nd, __str, __fn); \
        } \
    } while(0)

/* set 't'hingsdb event, used for thingsdb scope */
#define qbind__tev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X(qbind__set_thingsdb_event, __q, __nd, __str, __fn)
/* 'z'ero (no) event, used for node and thingsdb scope */
#define qbind__zev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X((void), __q, __nd, __str, __fn)

/* set 'b'oth thingsdb and collection event */
#define qbind__bev_fn(__q, __nd, __str, __fn) \
        SYNTAX__X(qbind__set_both_event, __q, __nd, __str, __fn)

static inline void qbind__set_collection_event(ti_qbind_t * qbind)
{
    qbind->flags |= qbind->flags & TI_QBIND_FLAG_COLLECTION
            ? TI_QBIND_FLAG_EVENT : 0;
}

static inline void qbind__set_thingsdb_event(ti_qbind_t * qbind)
{
    qbind->flags |= qbind->flags & TI_QBIND_FLAG_THINGSDB
            ? TI_QBIND_FLAG_EVENT : 0;
}

static inline void qbind__set_both_event(ti_qbind_t * qbind)
{
    qbind->flags |= qbind->flags & (
                TI_QBIND_FLAG_THINGSDB|TI_QBIND_FLAG_COLLECTION
            ) ? TI_QBIND_FLAG_EVENT : 0;
}

static void qbind__statement(ti_qbind_t * qbind, cleri_node_t * nd);

enum
{
    FN__CHAIN   = 1<<0,
    FN__ON_VAR  = TI_QBIND_FLAG_ON_VAR,
};

static void qbind__map_root_fn(ti_qbind_t * q, cleri_node_t * nd)
{
    /* a function name has at least size 1 */
    switch ((ti_alpha_lower_t) *nd->str)
    {
    case 'a':
        switch (nd->len)
        {
        case 6:
            qbind__nev_fn(q, nd, "assert", do__f_assert);
            break;
        case 8:
            qbind__nev_fn(q, nd, "auth_err", do__f_auth_err);
            break;
        case 10:
            qbind__nev_fn(q, nd, "assert_err", do__f_assert_err);
            break;
        }
        break;
    case 'b':
        switch (nd->len)
        {
        case 4:
            qbind__nev_fn(q, nd, "bool", do__f_bool);
            break;
        case 5:
            qbind__nev_fn(q, nd, "bytes", do__f_bytes);
            break;
        case 11:
            qbind__zev_fn(q, nd, "backup_info", do__f_backup_info);
            break;
        case 12:
            qbind__nev_fn(q, nd, "bad_data_err", do__f_bad_data_err);
            qbind__zev_fn(q, nd, "backups_info", do__f_backups_info);
            break;
        case 13:
            qbind__nev_fn(q, nd, "base64_decode", do__f_base64_decode);
            qbind__nev_fn(q, nd, "base64_encode", do__f_base64_encode);
            break;
        }
        break;
    case 'c':
        switch (nd->len)
        {
        case 8:
            qbind__zev_fn(q, nd, "counters", do__f_counters);
            break;
        case 15:
            qbind__zev_fn(q, nd, "collection_info", do__f_collection_info);
            break;
        case 16:
            qbind__zev_fn(q, nd, "collections_info", do__f_collections_info);
            break;
        }
        break;
    case 'd':
        switch (nd->len)
        {
        case 4:
            qbind__nev_fn(q, nd, "deep", do__f_deep);
            break;
        case 8:
            qbind__cev_fn(q, nd, "del_type", do__f_del_type);
            qbind__tev_fn(q, nd, "del_user", do__f_del_user);
            qbind__tev_fn(q, nd, "del_node", do__f_del_node);
            break;
        case 9:
            qbind__tev_fn(q, nd, "del_token", do__f_del_token);
            break;
        case 10:
            qbind__zev_fn(q, nd, "del_backup", do__f_del_backup);
            break;
        case 11:
            qbind__tev_fn(q, nd, "del_expired", do__f_del_expired);
            break;
        case 13:
            qbind__bev_fn(q, nd, "del_procedure", do__f_del_procedure);
            break;
        case 14:
            qbind__tev_fn(q, nd, "del_collection", do__f_del_collection);
            break;
        }
        break;
    case 'e':
        switch (nd->len)
        {
        case 3:
            qbind__nev_fn(q, nd, "err", do__f_err);
            break;
        }
        break;
    case 'f':
        switch (nd->len)
        {
        case 5:
            qbind__nev_fn(q, nd, "float", do__f_float);
            break;
        case 13:
            qbind__nev_fn(q, nd, "forbidden_err", do__f_forbidden_err);
            break;
        }
        break;
    case 'g':
        if (nd->len == 5)
            qbind__tev_fn(q, nd, "grant", do__f_grant);
        break;
    case 'h':
        switch (nd->len)
        {
        case 8:
            qbind__nev_fn(q, nd, "has_node", do__f_has_node);
            qbind__nev_fn(q, nd, "has_type", do__f_has_type);
            qbind__nev_fn(q, nd, "has_user", do__f_has_user);
            break;
        case 9:
            qbind__nev_fn(q, nd, "has_token", do__f_has_token);
            break;
        case 10:
            qbind__nev_fn(q, nd, "has_backup", do__f_has_backup);
            break;
        case 13:
            qbind__nev_fn(q, nd, "has_procedure", do__f_has_procedure);
            break;
        case 14:
            qbind__nev_fn(q, nd, "has_collection", do__f_has_collection);
            break;
        }
        break;
    case 'i':
        switch (nd->len)
        {
        case 2:
            qbind__nev_fn(q, nd, "if", do__f_if);
            break;
        case 3:
            qbind__nev_fn(q, nd, "int", do__f_int);
            break;
        case 5:
            qbind__nev_fn(q, nd, "iserr", do__f_iserr);
            qbind__nev_fn(q, nd, "isinf", do__f_isinf);
            qbind__nev_fn(q, nd, "isint", do__f_isint);
            qbind__nev_fn(q, nd, "isnan", do__f_isnan);
            qbind__nev_fn(q, nd, "isnil", do__f_isnil);
            qbind__nev_fn(q, nd, "israw", do__f_israw);
            qbind__nev_fn(q, nd, "isset", do__f_isset);
            qbind__nev_fn(q, nd, "isstr", do__f_isstr);
            break;
        case 6:
            qbind__nev_fn(q, nd, "isbool", do__f_isbool);
            qbind__nev_fn(q, nd, "islist", do__f_islist);
            qbind__nev_fn(q, nd, "isutf8", do__f_isutf8);
            break;
        case 7:
            qbind__nev_fn(q, nd, "isarray", do__f_isarray);
            qbind__nev_fn(q, nd, "isascii", do__f_isascii);
            qbind__nev_fn(q, nd, "isbytes", do__f_isbytes);
            qbind__nev_fn(q, nd, "isfloat", do__f_isfloat);
            qbind__nev_fn(q, nd, "isthing", do__f_isthing);
            qbind__nev_fn(q, nd, "istuple", do__f_istuple);
        }
        break;
    case 'j':
        break;
    case 'k':
        break;
    case 'l':
        switch (nd->len)
        {
        case 4:
            qbind__nev_fn(q, nd, "list", do__f_list);
            break;
        case 10:
            qbind__nev_fn(q, nd, "lookup_err", do__f_lookup_err);
            break;
        }
        break;
    case 'm':
        switch (nd->len)
        {
        case 8:
            qbind__cev_fn(q, nd, "mod_type", do__f_mod_type);
            break;
        case 13:
            qbind__nev_fn(q, nd, "max_quota_err", do__f_max_quota_err);
            break;
        }
        break;
    case 'n':
        switch (nd->len)
        {
        case 3:
            qbind__nev_fn(q, nd, "new", do__f_new);  /* most frequent used */
            qbind__nev_fn(q, nd, "now", do__f_now);
            break;
        case 8:
            qbind__cev_fn(q, nd, "new_type", do__f_new_type);
            qbind__nev_fn(q, nd, "node_err", do__f_node_err);
            qbind__tev_fn(q, nd, "new_node", do__f_new_node);
            qbind__tev_fn(q, nd, "new_user", do__f_new_user);
            break;
        case 9:
            qbind__tev_fn(q, nd, "new_token", do__f_new_token);
            qbind__zev_fn(q, nd, "node_info", do__f_node_info);
            break;
        case 10:
            qbind__zev_fn(q, nd, "new_backup", do__f_new_backup);
            qbind__zev_fn(q, nd, "nodes_info", do__f_nodes_info);
            break;
        case 13:
            qbind__bev_fn(q, nd, "new_procedure", do__f_new_procedure);
            break;
        case 14:
            qbind__tev_fn(q, nd, "new_collection", do__f_new_collection);
            break;
        case 17:
            qbind__nev_fn(q, nd, "num_arguments_err", do__f_num_arguments_err);
            break;
        }
        break;
    case 'o':
        switch (nd->len)
        {
        case 12:
            qbind__nev_fn(q, nd, "overflow_err", do__f_overflow_err);
            break;
        case 13:
            qbind__nev_fn(q, nd, "operation_err", do__f_operation_err);
            break;
        }
        break;
    case 'p':
        switch (nd->len)
        {
        case 13:
            qbind__nev_fn(q, nd, "procedure_doc", do__f_procedure_doc);
            break;
        case 14:
            qbind__nev_fn(q, nd, "procedure_info", do__f_procedure_info);
            break;
        case 15:
            qbind__nev_fn(q, nd, "procedures_info", do__f_procedures_info);
            break;
        }
        break;
    case 'q':
        break;
    case 'r':
        switch (nd->len)
        {
        case 3:
            qbind__nev_fn(q, nd, "run", do__f_run);
            break;
        case 4:
            qbind__nev_fn(q, nd, "rand", do__f_rand);
            qbind__nev_fn(q, nd, "refs", do__f_refs);
            break;
        case 5:
            qbind__nev_fn(q, nd, "raise", do__f_raise);
            qbind__tev_fn(q, nd, "revoke", do__f_revoke);
            break;
        case 6:
            qbind__nev_fn(q, nd, "return", do__f_return);
            break;
        case 7:
            qbind__nev_fn(q, nd, "randint", do__f_randint);
            break;
        case 11:
            qbind__tev_fn(q, nd, "rename_user", do__f_rename_user);
            break;
        case 14:
            qbind__zev_fn(q, nd, "reset_counters", do__f_reset_counters);
            break;
        case 17:
            qbind__tev_fn(q, nd, "rename_collection", do__f_rename_collection);
            break;
        }
        break;
    case 's':
        switch (nd->len)
        {
        case 3:
            qbind__nev_fn(q, nd, "set", do__f_set);
            qbind__nev_fn(q, nd, "str", do__f_str);
            break;
        case 8:
            qbind__cev_fn(q, nd, "set_type", do__f_set_type);
            qbind__zev_fn(q, nd, "shutdown", do__f_shutdown);
            break;
        case 10:
            qbind__nev_fn(q, nd, "syntax_err", do__f_syntax_err);
            break;
        case 12:
            qbind__tev_fn(q, nd, "set_password", do__f_set_password);
            break;
        case 13:
            qbind__zev_fn(q, nd, "set_log_level", do__f_set_log_level);
            break;
        }
        break;
    case 't':
        switch (nd->len)
        {
        case 3:
            qbind__nev_fn(q, nd, "try", do__f_try);
            break;
        case 4:
            qbind__nev_fn(q, nd, "type", do__f_type);
            break;
        case 5:
            qbind__nev_fn(q, nd, "thing", do__f_thing);  /* most frequent used */
            break;
        case 8:
            qbind__nev_fn(q, nd, "type_err", do__f_type_err);
            break;
        case 9:
            qbind__nev_fn(q, nd, "type_info", do__f_type_info);
            break;
        case 10:
            qbind__nev_fn(q, nd, "type_count", do__f_type_count);
            qbind__nev_fn(q, nd, "types_info", do__f_types_info);
            break;
        }
        break;
    case 'u':
        switch (nd->len)
        {
        case 9:
            qbind__zev_fn(q, nd, "user_info", do__f_user_info);
            break;
        case 10:
            qbind__zev_fn(q, nd, "users_info", do__f_users_info);
            break;
        }
        break;
    case 'v':
        if (nd->len == 9)
            qbind__nev_fn(q, nd, "value_err", do__f_value_err);
        break;
    case 'w':
        if (nd->len == 3)
            qbind__bev_fn(q, nd, "wse", do__f_wse);
        break;
    case 'x':
        break;
    case 'y':
        break;
    case 'z':
        if (nd->len == 12)
            qbind__nev_fn(q, nd, "zero_div_err", do__f_zero_div_err);
        break;
    }

    nd->data = NULL;  /* unknown function */
}

static void qbind__map_chained_fn(ti_qbind_t * q, cleri_node_t * nd, int flags)
{
    /* a function name has at least size 1 */
    switch ((ti_alpha_lower_t) *nd->str)
    {
    case 'a':
        if (nd->len == 3)
            qbind__vev_fn(q, nd, "add", do__f_add, flags);
        break;
    case 'b':
        break;
    case 'c':
        switch (nd->len)
        {
        case 4:
            qbind__nev_fn(q, nd, "call", do__f_call);
            qbind__nev_fn(q, nd, "code", do__f_code);
            break;
        case 6:
            qbind__nev_fn(q, nd, "choice", do__f_choice);
            break;
        case 8:
            qbind__nev_fn(q, nd, "contains", do__f_contains);
            break;
        }
        break;
    case 'd':
        if (nd->len == 3)
        {
            qbind__cev_fn(q, nd, "del", do__f_del);  /* most frequent used */
            qbind__nev_fn(q, nd, "doc", do__f_doc);
        }
        break;
    case 'e':
        switch (nd->len)
        {
        case 6:
            qbind__vev_fn(q, nd, "extend", do__f_extend, flags);
            break;
        case 8:
            qbind__nev_fn(q, nd, "endswith", do__f_endswith);
            break;
        }
        break;
    case 'f':
        switch (nd->len)
        {
        case 4:
            qbind__nev_fn(q, nd, "find", do__f_find);
            break;
        case 6:
            qbind__nev_fn(q, nd, "filter", do__f_filter);
            break;
        case 9:
            qbind__nev_fn(q, nd, "findindex", do__f_findindex);
            break;
        }
        break;
    case 'g':
        if (nd->len == 3)
            qbind__nev_fn(q, nd, "get", do__f_get);
        break;
    case 'h':
        if (nd->len == 3)
            qbind__nev_fn(q, nd, "has", do__f_has);
        break;
    case 'i':
        switch (nd->len)
        {
        case 2:
            qbind__nev_fn(q, nd, "id", do__f_id);
            break;
        case 7:
            qbind__nev_fn(q, nd, "indexof", do__f_indexof);
            break;
        }
        break;
    case 'j':
        break;
    case 'k':
        if (nd->len == 4)
            qbind__nev_fn(q, nd, "keys", do__f_keys);
        break;
    case 'l':
        switch (nd->len)
        {
        case 3:
            qbind__nev_fn(q, nd, "len", do__f_len);
            break;
        case 5:
            qbind__nev_fn(q, nd, "lower", do__f_lower);
            break;
        }
        break;
    case 'm':
        qbind__nev_fn(q, nd, "map", do__f_map);  /* most frequent used */
        qbind__nev_fn(q, nd, "msg", do__f_msg);
        break;
    case 'n':
        break;
    case 'o':
        break;
    case 'p':
        switch (nd->len)
        {
        case 3:
            qbind__vev_fn(q, nd, "pop", do__f_pop, flags);
            break;
        case 4:
            qbind__vev_fn(q, nd, "push", do__f_push, flags);
            break;
        }
        break;
    case 'q':
        break;
    case 'r':
        if (nd->len == 6)
            qbind__vev_fn(q, nd, "remove", do__f_remove, flags);
        break;
    case 's':
        switch (nd->len)
        {
        case 3:
            qbind__cev_fn(q, nd, "set", do__f_set);
            break;
        case 4:
            qbind__nev_fn(q, nd, "sort", do__f_sort);
            break;
        case 6:
            qbind__vev_fn(q, nd, "splice", do__f_splice, flags);
            break;
        case 10:
            qbind__nev_fn(q, nd, "startswith", do__f_startswith);
            break;
        }
        break;
    case 't':
        if (nd->len == 4)
            qbind__nev_fn(q, nd, "test", do__f_test);
        break;
    case 'u':
        switch (nd->len)
        {
        case 6:
            qbind__nev_fn(q, nd, "unwrap", do__f_unwrap);
            break;
        case 5:
            qbind__nev_fn(q, nd, "upper", do__f_upper);
            break;
        }
        break;
    case 'v':
        if (nd->len == 6)
            qbind__nev_fn(q, nd, "values", do__f_values);
        break;
    case 'w':
        if (nd->len == 4)
            qbind__nev_fn(q, nd, "wrap", do__f_wrap);
        break;
    case 'x':
    case 'y':
    case 'z':
        break;
    }

    nd->data = NULL;  /* unknown function */
}

static _Bool qbind__operations(
        ti_qbind_t * qbind,
        cleri_children_t * parent,
        uint32_t parent_gid)
{
    uint32_t gid = parent->node->children->next->node->cl_obj->gid;
    cleri_children_t * childb = parent->node->children->next->next;

    assert (gid >= CLERI_GID_OPR0_MUL_DIV_MOD &&
            gid <= CLERI_GID_OPR8_TERNARY);

    qbind__statement(qbind, parent->node->children->node);

    if (gid == CLERI_GID_OPR8_TERNARY)
        qbind__statement(
                qbind,
                parent->node->children->next->node->children->next->node);

    if (childb->node->children->node->cl_obj->gid != CLERI_GID_OPERATIONS)
    {
        qbind__statement(qbind, childb->node);
    }
    else if (qbind__operations(qbind, childb->node->children, gid))
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

static void qbind__function(ti_qbind_t * qbind, cleri_node_t * nd, int flags)
{
    intptr_t nargs = 0;
    cleri_children_t * child;

    /* map function to node */
    if (flags & FN__CHAIN)
        qbind__map_chained_fn(qbind, nd->children->node, flags);
    else
        qbind__map_root_fn(qbind, nd->children->node);

    /* list (arguments) */
    nd = nd->children->next->node->children->next->node;

    /* investigate arguments */
    for(child = nd->children;
        child;
        child = child->next ? child->next->next : NULL, ++nargs)
    {
        qbind__statement(qbind, child->node);  /* statement */
    }

    /* bind `nargs` to node->data */
    nd->data = (void *) nargs;
}

static void qbind__index(ti_qbind_t * qbind, cleri_node_t * nd)
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
            qbind__set_collection_event(qbind);
            qbind__statement(qbind, child->node           /* sequence */
                    ->children->next->next->next->node      /* assignment */
                    ->children->next->node);                /* statement */
        }

        for (; c; c = c->next)
            if (c->node->cl_obj->gid == CLERI_GID_STATEMENT)
                qbind__statement(qbind, c->node);
    }
    while ((child = child->next));
}

static inline void qbind__thing(ti_qbind_t * qbind, cleri_node_t * nd)
{
    uintptr_t sz = 0;
    cleri_children_t * child = nd           /* sequence */
            ->children->next->node          /* list */
            ->children;
    for (; child; child = child->next->next)
    {
        /* sequence(name: statement) (only investigate the statements */
        qbind__statement(
                qbind,
                child->node->children->next->next->node);  /* statement */
        ++sz;
        if (!child->next)
            break;
    }
    nd->data = (void *) sz;
}


static void qbind__var_opt_fa(ti_qbind_t * qbind, cleri_node_t * nd)
{
    if (nd->children->next)
    {
        switch (nd->children->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            qbind__function(qbind, nd, 0);
            return;
        case CLERI_GID_ASSIGN:
            qbind__statement(
                    qbind,
                    nd->children->next->node->children->next->node);
            break;
        case CLERI_GID_INSTANCE:
            qbind__thing(qbind, nd->children->next->node);
            return;
        default:
            assert (0);
            return;
        }
    }
    else
        qbind->flags |= TI_QBIND_FLAG_ON_VAR;

    nd->children->node->data = NULL;
    ++qbind->val_cache_n;
}

static void qbind__name_opt_fa(ti_qbind_t * qbind, cleri_node_t * nd)
{
    if (nd->children->next)
    {
        switch (nd->children->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            qbind__function(
                    qbind,
                    nd,
                    FN__CHAIN | (qbind->flags & TI_QBIND_FLAG_ON_VAR));
            return;
        case CLERI_GID_ASSIGN:
            qbind__set_collection_event(qbind);
            qbind__statement(
                    qbind,
                    nd->children->next->node->children->next->node);
            break;
        default:
            assert (0);
            return;
        }
    }
    nd->children->node->data = NULL;
    ++qbind->val_cache_n;
}

static inline void qbind__chain(ti_qbind_t * qbind, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);

    qbind__name_opt_fa(qbind, nd->children->next->node);

    qbind->flags &= ~TI_QBIND_FLAG_ON_VAR;

    /* index */
    if (nd->children->next->next->node->children)
        qbind__index(qbind, nd->children->next->next->node);

    /* chain */
    if (nd->children->next->next->next)
        qbind__chain(qbind, nd->children->next->next->next->node);
}

static void qbind__expr_choice(ti_qbind_t * qbind, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        qbind__chain(qbind, nd);        /* chain */
        return;
    case CLERI_GID_THING_BY_ID:
        nd->data = NULL;
        ++qbind->val_cache_n;
        return;
    case CLERI_GID_IMMUTABLE:
        nd = nd->children->node;
        switch (nd->cl_obj->gid)
        {
        case CLERI_GID_T_CLOSURE:
            /* investigate the statement, the rest can be skipped */
            qbind__statement(
                    qbind,
                    nd->children->next->next->next->node);
            /* fall through */
        case CLERI_GID_T_INT:
        case CLERI_GID_T_FLOAT:
        case CLERI_GID_T_STRING:
        case CLERI_GID_T_REGEX:
            ++qbind->val_cache_n;
            nd->data = NULL;        /* initialize data to null */
        }
        return;
    case CLERI_GID_VAR_OPT_MORE:
        qbind__var_opt_fa(qbind, nd);
        return;
    case CLERI_GID_THING:
        qbind__thing(qbind, nd);
        return;
    case CLERI_GID_ARRAY:
    {
        uintptr_t sz = 0;
        cleri_children_t * child = nd          /* sequence */
                ->children->next->node         /* list */
                ->children;
        for (; child; child = child->next ? child->next->next : NULL, ++sz)
            qbind__statement(qbind, child->node);  /* statement */
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
            qbind__statement(qbind, child->node);  /* statement */
        }
        while (child->next && (child = child->next->next));
        return;
    }
    case CLERI_GID_PARENTHESIS:
        qbind__statement(qbind, nd->children->next->node);
        return;
    }
    assert (0);
}

static inline void qbind__expression(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_children_t * child;
    intptr_t nots = 0;
    assert (nd->cl_obj->gid == CLERI_GID_EXPRESSION);

    for (child = nd->children->node->children; child; child = child->next)
        ++nots;

    nd->children->node->data = (void *) nots;

    qbind__expr_choice(qbind, nd->children->next->node);

    /* index */
    if (nd->children->next->next->node->children)
        qbind__index(qbind, nd->children->next->next->node);

    /* chain */
    if (nd->children->next->next->next)
        qbind__chain(qbind, nd->children->next->next->next->node);
}

static void qbind__statement(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_node_t * node;
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT);

    node = nd->children->node;
    switch (node->cl_obj->gid)
    {
    case CLERI_GID_EXPRESSION:
        qbind->flags &= ~TI_QBIND_FLAG_ON_VAR;
        qbind__expression(qbind, node);
        return;
    case CLERI_GID_OPERATIONS:
        qbind__operations(qbind, nd->children, 0);
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
 * - event requirement (set as qbind flags)
 * - closure detection and event requirement (set as flag to node->data)
 * - re-arrange operations to honor compare order
 * - count primitives cache requirements
 */
void ti_qbind_probe(ti_qbind_t * qbind, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT ||
            nd->cl_obj->gid == CLERI_GID_STATEMENTS);

    if (nd->cl_obj->gid == CLERI_GID_STATEMENTS)
    {
        cleri_children_t * child = nd->children;

        for (; child; child = child->next->next)
        {
            qbind__statement(qbind, child->node);   /* statement */

            if (!child->next)
                return;
        }
        return;
    }
    return qbind__statement(qbind, nd);
}
