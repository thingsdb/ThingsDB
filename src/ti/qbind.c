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

#define qbind__set_collection_event(__f) \
    (__f) |= (((__f) & TI_QBIND_FLAG_COLLECTION) && 1) << TI_QBIND_BIT_EVENT

static void qbind__statement(ti_qbind_t * qbind, cleri_node_t * nd);

#define qbind__flag_conv(__flag) \
    ((((__flag) & TI_QBIND_FLAG_ON_VAR) && 1)<<FN__BIT_ON_VAR)

typedef enum
{
    FN__BIT_CHAIN,
    FN__BIT_ON_VAR,
} qbind__fn_bit_t;

typedef enum
{
    FN__FLAG_CHAIN   = 1<<FN__BIT_CHAIN,
    FN__FLAG_ON_VAR  = 1<<FN__BIT_ON_VAR,
} qbind__fn_flag_t;

typedef struct
{
    char name[18];
    fn_cb fn;
    int e_flags;
    qbind__fn_flag_t q_flags;
    size_t n;
} qbind__fn_t;

#define ROOT_NE \
        .e_flags=0, .q_flags=0
#define ROOT_BE \
        .e_flags=TI_QBIND_FLAG_THINGSDB|TI_QBIND_FLAG_COLLECTION, .q_flags=0
#define ROOT_CE \
        .e_flags=TI_QBIND_FLAG_COLLECTION, .q_flags=0
#define ROOT_TE \
        .e_flags=TI_QBIND_FLAG_THINGSDB, .q_flags=0
#define CHAIN_NE \
        .e_flags=0, .q_flags=FN__FLAG_CHAIN
#define CHAIN_CE \
        .e_flags=TI_QBIND_FLAG_COLLECTION, .q_flags=FN__FLAG_CHAIN
#define CHAIN_NE_VAR \
        .e_flags=TI_QBIND_FLAG_COLLECTION, .q_flags=FN__FLAG_CHAIN|FN__FLAG_ON_VAR

#define QBIND__NUM_FN 136

qbind__fn_t fn_mapping[QBIND__NUM_FN] = {
    {.name="add",               .fn=do__f_add,                  CHAIN_NE_VAR},
    {.name="assert_err",        .fn=do__f_assert_err,           ROOT_NE},
    {.name="assert",            .fn=do__f_assert,               ROOT_NE},
    {.name="auth_err",          .fn=do__f_auth_err,             ROOT_NE},
    {.name="backup_info",       .fn=do__f_backup_info,          ROOT_NE},
    {.name="backups_info",      .fn=do__f_backups_info,         ROOT_NE},
    {.name="bad_data_err",      .fn=do__f_bad_data_err,         ROOT_NE},
    {.name="base64_decode",     .fn=do__f_base64_decode,        ROOT_NE},
    {.name="base64_encode",     .fn=do__f_base64_encode,        ROOT_NE},
    {.name="bool",              .fn=do__f_bool,                 ROOT_NE},
    {.name="bytes",             .fn=do__f_bytes,                ROOT_NE},
    {.name="call",              .fn=do__f_call,                 CHAIN_NE},
    {.name="choice",            .fn=do__f_choice,               CHAIN_NE},
    {.name="code",              .fn=do__f_code,                 CHAIN_NE},
    {.name="collection_info",   .fn=do__f_collection_info,      ROOT_NE},
    {.name="collections_info",  .fn=do__f_collections_info,     ROOT_NE},
    {.name="contains",          .fn=do__f_contains,             CHAIN_NE},
    {.name="counters",          .fn=do__f_counters,             ROOT_NE},
    {.name="deep",              .fn=do__f_deep,                 ROOT_NE},
    {.name="del_backup",        .fn=do__f_del_backup,           ROOT_NE},
    {.name="del_collection",    .fn=do__f_del_collection,       ROOT_TE},
    {.name="del_expired",       .fn=do__f_del_expired,          ROOT_TE},
    {.name="del_node",          .fn=do__f_del_node,             ROOT_TE},
    {.name="del_procedure",     .fn=do__f_del_procedure,        ROOT_BE},
    {.name="del_token",         .fn=do__f_del_token,            ROOT_TE},
    {.name="del_type",          .fn=do__f_del_type,             ROOT_CE},
    {.name="del_user",          .fn=do__f_del_user,             ROOT_TE},
    {.name="del",               .fn=do__f_del,                  CHAIN_CE},
    {.name="doc",               .fn=do__f_doc,                  CHAIN_NE},
    {.name="endswith",          .fn=do__f_endswith,             CHAIN_NE},
    {.name="err",               .fn=do__f_err,                  ROOT_NE},
    {.name="extend",            .fn=do__f_extend,               CHAIN_NE_VAR},
    {.name="filter",            .fn=do__f_filter,               CHAIN_NE},
    {.name="find",              .fn=do__f_find,                 CHAIN_NE},
    {.name="findindex",         .fn=do__f_findindex,            CHAIN_NE},
    {.name="float",             .fn=do__f_float,                ROOT_NE},
    {.name="forbidden_err",     .fn=do__f_forbidden_err,        ROOT_NE},
    {.name="get",               .fn=do__f_get,                  CHAIN_NE},
    {.name="grant",             .fn=do__f_grant,                ROOT_TE},
    {.name="has_backup",        .fn=do__f_has_backup,           ROOT_NE},
    {.name="has_collection",    .fn=do__f_has_collection,       ROOT_NE},
    {.name="has_node",          .fn=do__f_has_node,             ROOT_NE},
    {.name="has_procedure",     .fn=do__f_has_procedure,        ROOT_NE},
    {.name="has_token",         .fn=do__f_has_token,            ROOT_NE},
    {.name="has_type",          .fn=do__f_has_type,             ROOT_NE},
    {.name="has_user",          .fn=do__f_has_user,             ROOT_NE},
    {.name="has",               .fn=do__f_has,                  CHAIN_NE},
    {.name="id",                .fn=do__f_id,                   CHAIN_NE},
    {.name="if",                .fn=do__f_if,                   ROOT_NE},
    {.name="indexof",           .fn=do__f_indexof,              CHAIN_NE},
    {.name="int",               .fn=do__f_int,                  ROOT_NE},
    {.name="isarray",           .fn=do__f_isarray,              ROOT_NE},
    {.name="isascii",           .fn=do__f_isascii,              ROOT_NE},
    {.name="isbool",            .fn=do__f_isbool,               ROOT_NE},
    {.name="isbytes",           .fn=do__f_isbytes,              ROOT_NE},
    {.name="iserr",             .fn=do__f_iserr,                ROOT_NE},
    {.name="isfloat",           .fn=do__f_isfloat,              ROOT_NE},
    {.name="isinf",             .fn=do__f_isinf,                ROOT_NE},
    {.name="isint",             .fn=do__f_isint,                ROOT_NE},
    {.name="islist",            .fn=do__f_islist,               ROOT_NE},
    {.name="isnan",             .fn=do__f_isnan,                ROOT_NE},
    {.name="isnil",             .fn=do__f_isnil,                ROOT_NE},
    {.name="israw",             .fn=do__f_israw,                ROOT_NE},
    {.name="isset",             .fn=do__f_isset,                ROOT_NE},
    {.name="isstr",             .fn=do__f_isstr,                ROOT_NE},
    {.name="isthing",           .fn=do__f_isthing,              ROOT_NE},
    {.name="istuple",           .fn=do__f_istuple,              ROOT_NE},
    {.name="isutf8",            .fn=do__f_isutf8,               ROOT_NE},
    {.name="keys",              .fn=do__f_keys,                 CHAIN_NE},
    {.name="len",               .fn=do__f_len,                  CHAIN_NE},
    {.name="list",              .fn=do__f_list,                 ROOT_NE},
    {.name="lookup_err",        .fn=do__f_lookup_err,           ROOT_NE},
    {.name="lower",             .fn=do__f_lower,                CHAIN_NE},
    {.name="map",               .fn=do__f_map,                  CHAIN_NE},
    {.name="max_quota_err",     .fn=do__f_max_quota_err,        ROOT_NE},
    {.name="mod_type",          .fn=do__f_mod_type,             ROOT_CE},
    {.name="msg",               .fn=do__f_msg,                  CHAIN_NE},
    {.name="new_backup",        .fn=do__f_new_backup,           ROOT_NE},
    {.name="new_collection",    .fn=do__f_new_collection,       ROOT_TE},
    {.name="new_node",          .fn=do__f_new_node,             ROOT_TE},
    {.name="new_procedure",     .fn=do__f_new_procedure,        ROOT_BE},
    {.name="new_token",         .fn=do__f_new_token,            ROOT_TE},
    {.name="new_type",          .fn=do__f_new_type,             ROOT_CE},
    {.name="new_user",          .fn=do__f_new_user,             ROOT_TE},
    {.name="new",               .fn=do__f_new,                  ROOT_NE},
    {.name="node_err",          .fn=do__f_node_err,             ROOT_NE},
    {.name="node_info",         .fn=do__f_node_info,            ROOT_NE},
    {.name="nodes_info",        .fn=do__f_nodes_info,           ROOT_NE},
    {.name="now",               .fn=do__f_now,                  ROOT_NE},
    {.name="num_arguments_err", .fn=do__f_num_arguments_err,    ROOT_NE},
    {.name="operation_err",     .fn=do__f_operation_err,        ROOT_NE},
    {.name="overflow_err",      .fn=do__f_overflow_err,         ROOT_NE},
    {.name="pop",               .fn=do__f_pop,                  CHAIN_NE_VAR},
    {.name="procedure_doc",     .fn=do__f_procedure_doc,        ROOT_NE},
    {.name="procedure_info",    .fn=do__f_procedure_info,       ROOT_NE},
    {.name="procedures_info",   .fn=do__f_procedures_info,      ROOT_NE},
    {.name="push",              .fn=do__f_push,                 CHAIN_NE_VAR},
    {.name="raise",             .fn=do__f_raise,                ROOT_NE},
    {.name="rand",              .fn=do__f_rand,                 ROOT_NE},
    {.name="randint",           .fn=do__f_randint,              ROOT_NE},
    {.name="refs",              .fn=do__f_refs,                 ROOT_NE},
    {.name="remove",            .fn=do__f_remove,               CHAIN_NE_VAR},
    {.name="rename_collection", .fn=do__f_rename_collection,    ROOT_TE},
    {.name="rename_user",       .fn=do__f_rename_user,          ROOT_TE},
    {.name="reset_counters",    .fn=do__f_reset_counters,       ROOT_NE},
    {.name="return",            .fn=do__f_return,               ROOT_NE},
    {.name="revoke",            .fn=do__f_revoke,               ROOT_TE},
    {.name="run",               .fn=do__f_run,                  ROOT_NE},
    {.name="set_log_level",     .fn=do__f_set_log_level,        ROOT_NE},
    {.name="set_password",      .fn=do__f_set_password,         ROOT_TE},
    {.name="set_type",          .fn=do__f_set_type,             ROOT_CE},
    {.name="set",               .fn=do__f_set_property,         CHAIN_CE},
    {.name="set",               .fn=do__f_set_new_type,         ROOT_NE},
    {.name="shutdown",          .fn=do__f_shutdown,             ROOT_NE},
    {.name="sort",              .fn=do__f_sort,                 CHAIN_NE},
    {.name="splice",            .fn=do__f_splice,               CHAIN_NE_VAR},
    {.name="startswith",        .fn=do__f_startswith,           CHAIN_NE},
    {.name="str",               .fn=do__f_str,                  ROOT_NE},
    {.name="syntax_err",        .fn=do__f_syntax_err,           ROOT_NE},
    {.name="test",              .fn=do__f_test,                 CHAIN_NE},
    {.name="thing",             .fn=do__f_thing,                ROOT_NE},
    {.name="try",               .fn=do__f_try,                  ROOT_NE},
    {.name="type_count",        .fn=do__f_type_count,           ROOT_NE},
    {.name="type_err",          .fn=do__f_type_err,             ROOT_NE},
    {.name="type_info",         .fn=do__f_type_info,            ROOT_NE},
    {.name="type",              .fn=do__f_type,                 ROOT_NE},
    {.name="types_info",        .fn=do__f_types_info,           ROOT_NE},
    {.name="unwrap",            .fn=do__f_unwrap,               CHAIN_NE},
    {.name="upper",             .fn=do__f_upper,                CHAIN_NE},
    {.name="user_info",         .fn=do__f_user_info,            ROOT_NE},
    {.name="users_info",        .fn=do__f_users_info,           ROOT_NE},
    {.name="value_err",         .fn=do__f_value_err,            ROOT_NE},
    {.name="values",            .fn=do__f_values,               CHAIN_NE},
    {.name="wrap",              .fn=do__f_wrap,                 CHAIN_NE},
    {.name="wse",               .fn=do__f_wse,                  ROOT_BE},
    {.name="zero_div_err",      .fn=do__f_zero_div_err,         ROOT_NE},
};

static imap_t * qbind__imap;

/*
 * Hash function for the function bindings. This function is called at runtime
 * with user input but a length of at least one is still guaranteed.
 * If new functions are added, starting ThingsDB might fail if the computed
 * hash is no longer unique for each function. In this case it is save to make
 * some changes. If they compile unique, the function is good. Attempts should
 * be made to generate "low" key values since this will speed-up later lookups.
 */
static inline uint32_t qbind__fn_key(const char * s, const size_t n, int flags)
{
    uint16_t m = 0;
    size_t nn = n & 0b1111;
    for (; nn--; ++s)
        m += (*s & 0x1f) << (nn & 0x7);
    return ((m & 0x03ff) << 3) | ((n >> 1) ^ (flags & FN__FLAG_CHAIN));
}

int ti_qbind_init(void)
{
    int rc = 0;
    qbind__imap = imap_create();
    if (!qbind__imap)
        return -1;

    for (size_t i = 0, n = QBIND__NUM_FN; i < n; ++i)
    {
        uint32_t key;
        qbind__fn_t * fmap = &fn_mapping[i];

        fmap->n = strlen(fmap->name);
        key = qbind__fn_key(fmap->name, fmap->n, fmap->q_flags);
        rc = imap_add(qbind__imap, key, fmap);
        assert (rc == 0);  /* printf("k: %u, s: %s\n", key, fmap->name); */
        if (rc)
            return rc;
    }
    return 0;
}

void ti_qbind_destroy(void)
{
    imap_destroy(qbind__imap, NULL);
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

static void qbind__function(ti_qbind_t * q, cleri_node_t * nd, int flags)
{
    intptr_t nargs = 0;
    cleri_children_t * child;
    cleri_node_t * fnname = nd->children->node;

    uint32_t key = qbind__fn_key(fnname->str, fnname->len, flags);
    qbind__fn_t * fmap = imap_get(qbind__imap, key);

    fnname->data = (
            fmap &&
            fmap->n == fnname->len &&
            memcmp(fnname->str, fmap->name, fmap->n) == 0) ? fmap->fn : NULL;

    q->flags |= (
            fnname->data &&
            (q->flags & fmap->e_flags) &&
            ((~fmap->q_flags & FN__FLAG_ON_VAR) || (~flags & FN__FLAG_ON_VAR))
    ) << TI_QBIND_BIT_EVENT;

    /* list (arguments) */
    nd = nd->children->next->node->children->next->node;

    /* investigate arguments */
    for(child = nd->children;
        child;
        child = child->next ? child->next->next : NULL, ++nargs)
    {
        qbind__statement(q, child->node);  /* statement */
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
            qbind__set_collection_event(qbind->flags);
            qbind__statement(qbind, child->node             /* sequence */
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
                    FN__FLAG_CHAIN|qbind__flag_conv(qbind->flags));
            return;
        case CLERI_GID_ASSIGN:
            qbind__set_collection_event(qbind->flags);
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
        qbind->flags |= TI_QBIND_FLAG_ON_VAR;   /* enable var mode */
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
