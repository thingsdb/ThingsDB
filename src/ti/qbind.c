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

#include <ti/fn/fn.h>
#include <ti/fn/fnadd.h>
#include <ti/fn/fnaltraise.h>
#include <ti/fn/fnassert.h>
#include <ti/fn/fnassign.h>
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
#include <ti/fn/fndatetime.h>
#include <ti/fn/fndeep.h>
#include <ti/fn/fndef.h>
#include <ti/fn/fndel.h>
#include <ti/fn/fndelbackup.h>
#include <ti/fn/fndelcollection.h>
#include <ti/fn/fndelenum.h>
#include <ti/fn/fndelexpired.h>
#include <ti/fn/fndelmodule.h>
#include <ti/fn/fndelnode.h>
#include <ti/fn/fndelprocedure.h>
#include <ti/fn/fndeltoken.h>
#include <ti/fn/fndeltype.h>
#include <ti/fn/fndeluser.h>
#include <ti/fn/fndoc.h>
#include <ti/fn/fneach.h>
#include <ti/fn/fnelse.h>
#include <ti/fn/fnemit.h>
#include <ti/fn/fnendswith.h>
#include <ti/fn/fnenum.h>
#include <ti/fn/fnenuminfo.h>
#include <ti/fn/fnenumsinfo.h>
#include <ti/fn/fnerr.h>
#include <ti/fn/fnerrors.h>
#include <ti/fn/fneventid.h>
#include <ti/fn/fnevery.h>
#include <ti/fn/fnexport.h>
#include <ti/fn/fnextend.h>
#include <ti/fn/fnextract.h>
#include <ti/fn/fnfilter.h>
#include <ti/fn/fnfind.h>
#include <ti/fn/fnfindindex.h>
#include <ti/fn/fnfloat.h>
#include <ti/fn/fnformat.h>
#include <ti/fn/fnfuture.h>
#include <ti/fn/fnget.h>
#include <ti/fn/fngrant.h>
#include <ti/fn/fnhas.h>
#include <ti/fn/fnhasbackup.h>
#include <ti/fn/fnhascollection.h>
#include <ti/fn/fnhasenum.h>
#include <ti/fn/fnhasmodule.h>
#include <ti/fn/fnhasnode.h>
#include <ti/fn/fnhasprocedure.h>
#include <ti/fn/fnhastoken.h>
#include <ti/fn/fnhastype.h>
#include <ti/fn/fnhasuser.h>
#include <ti/fn/fnid.h>
#include <ti/fn/fnif.h>
#include <ti/fn/fnindexof.h>
#include <ti/fn/fnint.h>
#include <ti/fn/fnisarray.h>
#include <ti/fn/fnisascii.h>
#include <ti/fn/fnisbool.h>
#include <ti/fn/fnisbytes.h>
#include <ti/fn/fnisclosure.h>
#include <ti/fn/fnisdatetime.h>
#include <ti/fn/fnisenum.h>
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
#include <ti/fn/fnistimeval.h>
#include <ti/fn/fnistuple.h>
#include <ti/fn/fnisutf8.h>
#include <ti/fn/fnjoin.h>
#include <ti/fn/fnkeys.h>
#include <ti/fn/fnlen.h>
#include <ti/fn/fnlist.h>
#include <ti/fn/fnlower.h>
#include <ti/fn/fnmap.h>
#include <ti/fn/fnmodenum.h>
#include <ti/fn/fnmodtype.h>
#include <ti/fn/fnmoduleinfo.h>
#include <ti/fn/fnmodulesinfo.h>
#include <ti/fn/fnmove.h>
#include <ti/fn/fnmsg.h>
#include <ti/fn/fnname.h>
#include <ti/fn/fnnew.h>
#include <ti/fn/fnnewbackup.h>
#include <ti/fn/fnnewcollection.h>
#include <ti/fn/fnnewmodule.h>
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
#include <ti/fn/fnrandstr.h>
#include <ti/fn/fnrange.h>
#include <ti/fn/fnreduce.h>
#include <ti/fn/fnrefs.h>
#include <ti/fn/fnremove.h>
#include <ti/fn/fnrenamecollection.h>
#include <ti/fn/fnrenameenum.h>
#include <ti/fn/fnrenamemodule.h>
#include <ti/fn/fnrenameprocedure.h>
#include <ti/fn/fnrenametype.h>
#include <ti/fn/fnrenameuser.h>
#include <ti/fn/fnreplace.h>
#include <ti/fn/fnresetcounters.h>
#include <ti/fn/fnrestartmodule.h>
#include <ti/fn/fnrestore.h>
#include <ti/fn/fnreturn.h>
#include <ti/fn/fnreverse.h>
#include <ti/fn/fnrevoke.h>
#include <ti/fn/fnrun.h>
#include <ti/fn/fnset.h>
#include <ti/fn/fnsetenum.h>
#include <ti/fn/fnsetloglevel.h>
#include <ti/fn/fnsetmoduleconf.h>
#include <ti/fn/fnsetmodulescope.h>
#include <ti/fn/fnsetpassword.h>
#include <ti/fn/fnsettimezone.h>
#include <ti/fn/fnsettype.h>
#include <ti/fn/fnshift.h>
#include <ti/fn/fnshutdown.h>
#include <ti/fn/fnsome.h>
#include <ti/fn/fnsort.h>
#include <ti/fn/fnsplice.h>
#include <ti/fn/fnsplit.h>
#include <ti/fn/fnstartswith.h>
#include <ti/fn/fnstr.h>
#include <ti/fn/fntest.h>
#include <ti/fn/fnthen.h>
#include <ti/fn/fnthing.h>
#include <ti/fn/fntimezonesinfo.h>
#include <ti/fn/fnto.h>
#include <ti/fn/fntrim.h>
#include <ti/fn/fntrimleft.h>
#include <ti/fn/fntrimright.h>
#include <ti/fn/fntry.h>
#include <ti/fn/fntype.h>
#include <ti/fn/fntypeassert.h>
#include <ti/fn/fntypecount.h>
#include <ti/fn/fntypeinfo.h>
#include <ti/fn/fntypesinfo.h>
#include <ti/fn/fnunshift.h>
#include <ti/fn/fnunwatch.h>
#include <ti/fn/fnunwrap.h>
#include <ti/fn/fnupper.h>
#include <ti/fn/fnuserinfo.h>
#include <ti/fn/fnusersinfo.h>
#include <ti/fn/fnvalue.h>
#include <ti/fn/fnvalues.h>
#include <ti/fn/fnwatch.h>
#include <ti/fn/fnweek.h>
#include <ti/fn/fnweekday.h>
#include <ti/fn/fnwrap.h>
#include <ti/fn/fnwse.h>
#include <ti/fn/fnyday.h>
#include <ti/fn/fnzone.h>
#include <ti/qbind.h>
#include <ti/preopr.h>

#define qbind__set_collection_event(__f) \
    (__f) |= (((__f) & TI_QBIND_FLAG_COLLECTION) && 1) << TI_QBIND_BIT_EVENT

static void qbind__statement(ti_qbind_t * qbind, cleri_node_t * nd);


/*
 * Used to convert the TI_QBIND_FLAG_ON_VAR to FN__FLAG_EXCL_VAR. Therefore
 * the FN__FLAG_EXCL_VAR must be `1`.
 */
#define qbind__is_onvar(__flag) (((__flag) & TI_QBIND_FLAG_ON_VAR) && 1)

/*
 * The following `enum` and `asso_values` are generated using `gperf` and
 * the utility `pcregrep` to generate the input.
 *
 * Command:
 *
 *    pcregrep -o1 '\.name\=\"(\w+)' qbind.c | gperf -E -k '*,1,$' -m 200
 */
enum
{
    TOTAL_KEYWORDS = 201,
    MIN_WORD_LENGTH = 2,
    MAX_WORD_LENGTH = 17,
    MIN_HASH_VALUE = 22,
    MAX_HASH_VALUE = 489
};

static inline unsigned int qbind__hash(
        register const char * s,
        register size_t n)
{
    static unsigned short asso_values[] =
    {
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490,   9, 490,   9, 490,  11, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490,   9, 490,  10,  73,  45,
         11,   9,  55, 109,  74,   9,  10,  87,  45,   9,
         13,  25,  81,   9,  10,   9,  12,  12, 140, 130,
        159, 106,  29, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490, 490, 490, 490, 490,
        490, 490, 490, 490, 490, 490
    };

    register unsigned int hval = n;

    switch (hval)
    {
        default:
            hval += asso_values[(unsigned char)s[16]];
            /*fall through*/
        case 16:
            hval += asso_values[(unsigned char)s[15]];
            /*fall through*/
        case 15:
            hval += asso_values[(unsigned char)s[14]];
            /*fall through*/
        case 14:
            hval += asso_values[(unsigned char)s[13]];
            /*fall through*/
        case 13:
            hval += asso_values[(unsigned char)s[12]];
            /*fall through*/
        case 12:
            hval += asso_values[(unsigned char)s[11]];
            /*fall through*/
        case 11:
            hval += asso_values[(unsigned char)s[10]];
            /*fall through*/
        case 10:
            hval += asso_values[(unsigned char)s[9]];
            /*fall through*/
        case 9:
            hval += asso_values[(unsigned char)s[8]];
            /*fall through*/
        case 8:
            hval += asso_values[(unsigned char)s[7]];
            /*fall through*/
        case 7:
            hval += asso_values[(unsigned char)s[6]];
            /*fall through*/
        case 6:
            hval += asso_values[(unsigned char)s[5]];
            /*fall through*/
        case 5:
            hval += asso_values[(unsigned char)s[4]];
            /*fall through*/
        case 4:
            hval += asso_values[(unsigned char)s[3]];
            /*fall through*/
        case 3:
            hval += asso_values[(unsigned char)s[2]];
            /*fall through*/
        case 2:
            hval += asso_values[(unsigned char)s[1]];
            /*fall through*/
        case 1:
            hval += asso_values[(unsigned char)s[0]];
            break;
    }
    return hval;
}

typedef enum
{
    FN__FLAG_EXCL_VAR   = 1<<0, /* must be 1, see qbind__is_onvar() */
    FN__FLAG_EV_T       = 1<<1, /* must be 2, maps to ti_qbind_bit_t */
    FN__FLAG_EV_C       = 1<<2, /* must be 4, maps to ti_qbind_bit_t */
    FN__FLAG_ROOT       = 1<<3,
    FN__FLAG_CHAIN      = 1<<4,
    FN__FLAG_XROOT      = 1<<5,
    FN__FLAG_AS_ON_VAR  = 1<<6, /* function result as on variable */
    FN__FLAG_FUT        = 1<<7, /* must check for closure and do not set event
                                   by itself */
} qbind__fn_flag_t;

typedef struct
{
    char name[MAX_WORD_LENGTH+1];
    qbind__fn_flag_t flags;
    fn_cb fn;
    size_t n;
} qbind__fmap_t;

/*
 * Every function which might return a pointer to a mutable value, other than
 * a thing, must start with X... This is not true if you can guarantee that or,
 * the return value does not require an event, or if the function itself is
 * already making an event. (then the optimization has no meaning anyway)
 *
 * For example:
 *   t = .my_stored_thing;
 *   t.get('arr').push(42);  // Requires an event, arr is still a pointer.
 *
 *   arr = t.get('arr');
 *   arr.push(42);  // Does not require an event, since arr is assigned and
 *                  // therefore copied.
 */

#define ROOT_NE \
        .flags=FN__FLAG_ROOT|FN__FLAG_AS_ON_VAR
#define ROOT_BE \
        .flags=FN__FLAG_ROOT|FN__FLAG_EV_C|FN__FLAG_EV_T|FN__FLAG_AS_ON_VAR
#define ROOT_CE \
        .flags=FN__FLAG_ROOT|FN__FLAG_EV_C|FN__FLAG_AS_ON_VAR
#define ROOT_TE \
        .flags=FN__FLAG_ROOT|FN__FLAG_EV_T|FN__FLAG_AS_ON_VAR
#define CHAIN_NE \
        .flags=FN__FLAG_CHAIN|FN__FLAG_AS_ON_VAR
#define CHAIN_CE \
        .flags=FN__FLAG_CHAIN|FN__FLAG_EV_C|FN__FLAG_AS_ON_VAR
#define CHAIN_CE_XVAR \
        .flags=FN__FLAG_CHAIN|FN__FLAG_EXCL_VAR|FN__FLAG_EV_C|FN__FLAG_AS_ON_VAR
#define CHAIN_FUT \
        .flags=FN__FLAG_CHAIN|FN__FLAG_AS_ON_VAR|FN__FLAG_FUT
#define BOTH_CE_XROOT \
        .flags=FN__FLAG_ROOT|FN__FLAG_CHAIN|FN__FLAG_EV_C|FN__FLAG_XROOT|FN__FLAG_AS_ON_VAR
#define XROOT_NE \
        .flags=FN__FLAG_ROOT
#define XROOT_BE \
        .flags=FN__FLAG_ROOT|FN__FLAG_EV_C|FN__FLAG_EV_T
#define XROOT_CE \
        .flags=FN__FLAG_ROOT|FN__FLAG_EV_C
#define XROOT_TE \
        .flags=FN__FLAG_ROOT|FN__FLAG_EV_T
#define XCHAIN_NE \
        .flags=FN__FLAG_CHAIN
#define XCHAIN_CE \
        .flags=FN__FLAG_CHAIN|FN__FLAG_EV_C
#define XCHAIN_CE_XVAR \
        .flags=FN__FLAG_CHAIN|FN__FLAG_EXCL_VAR|FN__FLAG_EV_C
#define XBOTH_CE_XROOT \
        .flags=FN__FLAG_ROOT|FN__FLAG_CHAIN|FN__FLAG_EV_C|FN__FLAG_XROOT

qbind__fmap_t qbind__fn_mapping[TOTAL_KEYWORDS] = {
    {.name="add",               .fn=do__f_add,                  CHAIN_CE_XVAR},
    {.name="alt_raise",         .fn=do__f_alt_raise,            ROOT_NE},
    {.name="assert_err",        .fn=do__f_assert_err,           ROOT_NE},
    {.name="assert",            .fn=do__f_assert,               ROOT_NE},
    {.name="assign",            .fn=do__f_assign,               CHAIN_CE},
    {.name="auth_err",          .fn=do__f_auth_err,             ROOT_NE},
    {.name="backup_info",       .fn=do__f_backup_info,          ROOT_NE},
    {.name="backups_info",      .fn=do__f_backups_info,         ROOT_NE},
    {.name="bad_data_err",      .fn=do__f_bad_data_err,         ROOT_NE},
    {.name="base64_decode",     .fn=do__f_base64_decode,        ROOT_NE},
    {.name="base64_encode",     .fn=do__f_base64_encode,        ROOT_NE},
    {.name="bool",              .fn=do__f_bool,                 ROOT_NE},
    {.name="bytes",             .fn=do__f_bytes,                ROOT_NE},
    {.name="call",              .fn=do__f_call,                 XCHAIN_NE},
    {.name="cancelled_err",     .fn=do__f_cancelled_err,        ROOT_NE},
    {.name="choice",            .fn=do__f_choice,               CHAIN_NE},
    {.name="code",              .fn=do__f_code,                 CHAIN_NE},
    {.name="collection_info",   .fn=do__f_collection_info,      ROOT_NE},
    {.name="collections_info",  .fn=do__f_collections_info,     ROOT_NE},
    {.name="contains",          .fn=do__f_contains,             CHAIN_NE},
    {.name="counters",          .fn=do__f_counters,             ROOT_NE},
    {.name="datetime",          .fn=do__f_datetime,             ROOT_NE},
    {.name="deep",              .fn=do__f_deep,                 ROOT_NE},
    {.name="def",               .fn=do__f_def,                  CHAIN_NE},
    {.name="del_backup",        .fn=do__f_del_backup,           ROOT_NE},
    {.name="del_collection",    .fn=do__f_del_collection,       ROOT_TE},
    {.name="del_enum",          .fn=do__f_del_enum,             ROOT_CE},
    {.name="del_expired",       .fn=do__f_del_expired,          ROOT_TE},
    {.name="del_module",        .fn=do__f_del_module,           ROOT_TE},
    {.name="del_node",          .fn=do__f_del_node,             ROOT_TE},
    {.name="del_procedure",     .fn=do__f_del_procedure,        ROOT_BE},
    {.name="del_token",         .fn=do__f_del_token,            ROOT_TE},
    {.name="del_type",          .fn=do__f_del_type,             ROOT_CE},
    {.name="del_user",          .fn=do__f_del_user,             ROOT_TE},
    {.name="del",               .fn=do__f_del,                  CHAIN_CE},
    {.name="doc",               .fn=do__f_doc,                  CHAIN_NE},
    {.name="each",              .fn=do__f_each,                 CHAIN_NE},
    {.name="emit",              .fn=do__f_emit,                 CHAIN_CE},
    {.name="else",              .fn=do__f_else,                 CHAIN_FUT},
    {.name="ends_with",         .fn=do__f_ends_with,            CHAIN_NE},
    {.name="enum_info",         .fn=do__f_enum_info,            ROOT_NE},
    {.name="enum",              .fn=do__f_enum,                 ROOT_NE},
    {.name="enums_info",        .fn=do__f_enums_info,           ROOT_NE},
    {.name="err",               .fn=do__f_err,                  ROOT_NE},
    {.name="event_id",          .fn=do__f_event_id,             ROOT_NE},
    {.name="every",             .fn=do__f_every,                CHAIN_NE},
    {.name="export",            .fn=do__f_export,               ROOT_NE},
    {.name="extend",            .fn=do__f_extend,               CHAIN_CE_XVAR},
    {.name="extract",           .fn=do__f_extract,              CHAIN_NE},
    {.name="filter",            .fn=do__f_filter,               CHAIN_NE},
    {.name="find_index",        .fn=do__f_find_index,           CHAIN_NE},
    {.name="find",              .fn=do__f_find,                 XCHAIN_NE},
    {.name="float",             .fn=do__f_float,                ROOT_NE},
    {.name="forbidden_err",     .fn=do__f_forbidden_err,        ROOT_NE},
    {.name="format",            .fn=do__f_format,               CHAIN_NE},
    {.name="future",            .fn=do__f_future,               ROOT_NE},
    {.name="get",               .fn=do__f_get,                  XCHAIN_NE},
    {.name="grant",             .fn=do__f_grant,                ROOT_TE},
    {.name="has_backup",        .fn=do__f_has_backup,           ROOT_NE},
    {.name="has_collection",    .fn=do__f_has_collection,       ROOT_NE},
    {.name="has_enum",          .fn=do__f_has_enum,             ROOT_NE},
    {.name="has_module",        .fn=do__f_has_module,           ROOT_NE},
    {.name="has_node",          .fn=do__f_has_node,             ROOT_NE},
    {.name="has_procedure",     .fn=do__f_has_procedure,        ROOT_NE},
    {.name="has_token",         .fn=do__f_has_token,            ROOT_NE},
    {.name="has_type",          .fn=do__f_has_type,             ROOT_NE},
    {.name="has_user",          .fn=do__f_has_user,             ROOT_NE},
    {.name="has",               .fn=do__f_has,                  CHAIN_NE},
    {.name="id",                .fn=do__f_id,                   CHAIN_NE},
    {.name="if",                .fn=do__f_if,                   ROOT_NE},
    {.name="index_of",          .fn=do__f_index_of,             CHAIN_NE},
    {.name="int",               .fn=do__f_int,                  ROOT_NE},
    {.name="is_array",          .fn=do__f_is_array,             ROOT_NE},
    {.name="is_ascii",          .fn=do__f_is_ascii,             ROOT_NE},
    {.name="is_bool",           .fn=do__f_is_bool,              ROOT_NE},
    {.name="is_bytes",          .fn=do__f_is_bytes,             ROOT_NE},
    {.name="is_closure",        .fn=do__f_is_closure,           ROOT_NE},
    {.name="is_datetime",       .fn=do__f_is_datetime,          ROOT_NE},
    {.name="is_enum",           .fn=do__f_is_enum,              ROOT_NE},
    {.name="is_err",            .fn=do__f_is_err,               ROOT_NE},
    {.name="is_float",          .fn=do__f_is_float,             ROOT_NE},
    {.name="is_inf",            .fn=do__f_is_inf,               ROOT_NE},
    {.name="is_int",            .fn=do__f_is_int,               ROOT_NE},
    {.name="is_list",           .fn=do__f_is_list,              ROOT_NE},
    {.name="is_nan",            .fn=do__f_is_nan,               ROOT_NE},
    {.name="is_nil",            .fn=do__f_is_nil,               ROOT_NE},
    {.name="is_raw",            .fn=do__f_is_raw,               ROOT_NE},
    {.name="is_set",            .fn=do__f_is_set,               ROOT_NE},
    {.name="is_str",            .fn=do__f_is_str,               ROOT_NE},
    {.name="is_thing",          .fn=do__f_is_thing,             ROOT_NE},
    {.name="is_timeval",        .fn=do__f_is_timeval,           ROOT_NE},
    {.name="is_tuple",          .fn=do__f_is_tuple,             ROOT_NE},
    {.name="is_utf8",           .fn=do__f_is_utf8,              ROOT_NE},
    {.name="join",              .fn=do__f_join,                 CHAIN_NE},
    {.name="keys",              .fn=do__f_keys,                 CHAIN_NE},
    {.name="len",               .fn=do__f_len,                  CHAIN_NE},
    {.name="list",              .fn=do__f_list,                 ROOT_NE},
    {.name="lookup_err",        .fn=do__f_lookup_err,           ROOT_NE},
    {.name="lower",             .fn=do__f_lower,                CHAIN_NE},
    {.name="map",               .fn=do__f_map,                  CHAIN_NE},
    {.name="max_quota_err",     .fn=do__f_max_quota_err,        ROOT_NE},
    {.name="mod_enum",          .fn=do__f_mod_enum,             ROOT_CE},
    {.name="mod_type",          .fn=do__f_mod_type,             ROOT_CE},
    {.name="move",              .fn=do__f_move,                 CHAIN_NE},
    {.name="msg",               .fn=do__f_msg,                  CHAIN_NE},
    {.name="name",              .fn=do__f_name,                 CHAIN_NE},
    {.name="new_backup",        .fn=do__f_new_backup,           ROOT_NE},
    {.name="new_collection",    .fn=do__f_new_collection,       ROOT_TE},
    {.name="new_module",        .fn=do__f_new_module,           ROOT_TE},
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
    {.name="module_info",       .fn=do__f_module_info,          ROOT_NE},
    {.name="modules_info",      .fn=do__f_modules_info,         ROOT_NE},
    {.name="operation_err",     .fn=do__f_operation_err,        ROOT_NE},
    {.name="overflow_err",      .fn=do__f_overflow_err,         ROOT_NE},
    {.name="pop",               .fn=do__f_pop,                  CHAIN_CE_XVAR},
    {.name="procedure_doc",     .fn=do__f_procedure_doc,        ROOT_NE},
    {.name="procedure_info",    .fn=do__f_procedure_info,       ROOT_NE},
    {.name="procedures_info",   .fn=do__f_procedures_info,      ROOT_NE},
    {.name="push",              .fn=do__f_push,                 CHAIN_CE_XVAR},
    {.name="raise",             .fn=do__f_raise,                ROOT_NE},
    {.name="rand",              .fn=do__f_rand,                 ROOT_NE},
    {.name="randint",           .fn=do__f_randint,              ROOT_NE},
    {.name="randstr",           .fn=do__f_randstr,              ROOT_NE},
    {.name="range",             .fn=do__f_range,                ROOT_NE},
    {.name="reduce",            .fn=do__f_reduce,               XCHAIN_NE},
    {.name="refs",              .fn=do__f_refs,                 ROOT_NE},
    {.name="remove",            .fn=do__f_remove,               CHAIN_CE_XVAR},
    {.name="rename_collection", .fn=do__f_rename_collection,    ROOT_TE},
    {.name="rename_enum",       .fn=do__f_rename_enum,          ROOT_CE},
    {.name="rename_module",     .fn=do__f_rename_module,        ROOT_CE},
    {.name="rename_procedure",  .fn=do__f_rename_procedure,     ROOT_BE},
    {.name="rename_type",       .fn=do__f_rename_type,          ROOT_CE},
    {.name="rename_user",       .fn=do__f_rename_user,          ROOT_TE},
    {.name="replace",           .fn=do__f_replace,              CHAIN_NE},
    {.name="reset_counters",    .fn=do__f_reset_counters,       ROOT_NE},
    {.name="restart_module",    .fn=do__f_restart_module,       ROOT_NE},
    {.name="restore",           .fn=do__f_restore,              ROOT_TE},
    {.name="return",            .fn=do__f_return,               ROOT_NE},
    {.name="reverse",           .fn=do__f_reverse,              CHAIN_NE},
    {.name="revoke",            .fn=do__f_revoke,               ROOT_TE},
    {.name="run",               .fn=do__f_run,                  XROOT_NE},
    {.name="set_enum",          .fn=do__f_set_enum,             ROOT_CE},
    {.name="set_log_level",     .fn=do__f_set_log_level,        ROOT_NE},
    {.name="set_module_conf",   .fn=do__f_set_module_conf,      ROOT_TE},
    {.name="set_module_scope",  .fn=do__f_set_module_scope,     ROOT_TE},
    {.name="set_password",      .fn=do__f_set_password,         ROOT_TE},
    {.name="set_time_zone",     .fn=do__f_set_time_zone,        ROOT_TE},
    {.name="set_type",          .fn=do__f_set_type,             ROOT_CE},
    {.name="set",               .fn=do__f_set,                  BOTH_CE_XROOT},
    {.name="shift",             .fn=do__f_shift,                CHAIN_CE_XVAR},
    {.name="shutdown",          .fn=do__f_shutdown,             ROOT_NE},
    {.name="some",              .fn=do__f_some,                 CHAIN_NE},
    {.name="sort",              .fn=do__f_sort,                 CHAIN_NE},
    {.name="splice",            .fn=do__f_splice,               CHAIN_CE_XVAR},
    {.name="split",             .fn=do__f_split,                CHAIN_NE},
    {.name="starts_with",       .fn=do__f_starts_with,          CHAIN_NE},
    {.name="str",               .fn=do__f_str,                  ROOT_NE},
    {.name="syntax_err",        .fn=do__f_syntax_err,           ROOT_NE},
    {.name="test",              .fn=do__f_test,                 CHAIN_NE},
    {.name="then",              .fn=do__f_then,                 CHAIN_FUT},
    {.name="thing",             .fn=do__f_thing,                ROOT_NE},
    {.name="time_zones_info",   .fn=do__f_time_zones_info,      ROOT_NE},
    {.name="timeval",           .fn=do__f_timeval,              ROOT_NE},
    {.name="to",                .fn=do__f_to,                   CHAIN_NE},
    {.name="trim_left",         .fn=do__f_trim_left,            CHAIN_NE},
    {.name="trim_right",        .fn=do__f_trim_right,           CHAIN_NE},
    {.name="trim",              .fn=do__f_trim,                 CHAIN_NE},
    {.name="try",               .fn=do__f_try,                  XROOT_NE},
    {.name="type_assert",       .fn=do__f_type_assert,          ROOT_NE},
    {.name="type_count",        .fn=do__f_type_count,           ROOT_NE},
    {.name="type_err",          .fn=do__f_type_err,             ROOT_NE},
    {.name="type_info",         .fn=do__f_type_info,            ROOT_NE},
    {.name="type",              .fn=do__f_type,                 ROOT_NE},
    {.name="types_info",        .fn=do__f_types_info,           ROOT_NE},
    {.name="unshift",           .fn=do__f_unshift,              CHAIN_CE_XVAR},
    {.name="unwatch",           .fn=do__f_unwatch,              CHAIN_NE},
    {.name="unwrap",            .fn=do__f_unwrap,               CHAIN_NE},
    {.name="upper",             .fn=do__f_upper,                CHAIN_NE},
    {.name="user_info",         .fn=do__f_user_info,            ROOT_NE},
    {.name="users_info",        .fn=do__f_users_info,           ROOT_NE},
    {.name="value_err",         .fn=do__f_value_err,            ROOT_NE},
    {.name="value",             .fn=do__f_value,                CHAIN_NE},
    {.name="values",            .fn=do__f_values,               CHAIN_NE},
    {.name="watch",             .fn=do__f_watch,                CHAIN_NE},
    {.name="week",              .fn=do__f_week,                 CHAIN_NE},
    {.name="weekday",           .fn=do__f_weekday,              CHAIN_NE},
    {.name="wrap",              .fn=do__f_wrap,                 CHAIN_NE},
    {.name="wse",               .fn=do__f_wse,                  XROOT_BE},
    {.name="yday",              .fn=do__f_yday,                 CHAIN_NE},
    {.name="zero_div_err",      .fn=do__f_zero_div_err,         ROOT_NE},
    {.name="zone",              .fn=do__f_zone,                 CHAIN_NE},
};

static qbind__fmap_t * qbind__fn_map[MAX_HASH_VALUE+1];

void ti_qbind_init(void)
{
    for (size_t i = 0, n = TOTAL_KEYWORDS; i < n; ++i)
    {
        uint32_t key;
        qbind__fmap_t * fmap = &qbind__fn_mapping[i];

        fmap->n = strlen(fmap->name);
        key = qbind__hash(fmap->name, fmap->n);

        assert (qbind__fn_map[key] == NULL);
        assert (key <= MAX_HASH_VALUE);

        qbind__fn_map[key] = fmap;
    }
}

static _Bool qbind__swap(cleri_children_t * parent, uint32_t parent_gid)
{
    uint32_t gid = parent->node->children->next->node->cl_obj->gid;
    cleri_children_t * childb = parent->node->children->next->next;

    if (childb->node->children->node->cl_obj->gid == CLERI_GID_OPERATIONS &&
        qbind__swap(childb->node->children, gid))
    {
        /* Swap operations */
        cleri_children_t * syntax_childa;
        cleri_node_t * tmp = parent->node;  /* operations */
        parent->node = childb->node->children->node;  /* operations */

        gid = parent->node->children->next->node->cl_obj->gid;
        syntax_childa = parent->node->children->node->children;
        childb->node->children->node = syntax_childa->node;
        syntax_childa->node = tmp;

        /* Recursive swapping */
        qbind__swap(syntax_childa, gid);
    }

    return gid > parent_gid;
}

static _Bool qbind__operations(
        ti_qbind_t * qbind,
        cleri_children_t * parent,
        uint32_t parent_gid)
{
    uint32_t gid = parent->node->children->next->node->cl_obj->gid;
    cleri_children_t * childb = parent->node->children->next->next;

    parent->node->data = ti_do_operations;

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

        /* This is required for recursive swapping.
         * For example the order:
         *    C B A
         * One iteration will perform the following swaps:
         *    C A B
         *    A C B
         * But now we still need to swap C and B, therefore we require the
         * recursive swap. (nothing except swapping)
         */
        qbind__swap(syntax_childa, gid);
    }
    return gid > parent_gid;
}

static void qbind__peek_statement_for_closure(
        ti_qbind_t * q,
        cleri_node_t * nd)
{
    cleri_node_t * node;

    if ((node = nd->children->node)->cl_obj->gid == CLERI_GID_EXPRESSION &&
        node->children->next->node->cl_obj->gid == CLERI_GID_T_CLOSURE)
    {
        uint8_t no_ev_flag = ~q->flags & TI_QBIND_FLAG_EVENT;
        qbind__statement(q, nd);
        q->flags &= ~no_ev_flag;
        return;
    }

    qbind__statement(q, nd);  /* statement */
}

static void qbind__function(
        ti_qbind_t * q,
        cleri_node_t * nd,
        int flags)
{
    cleri_children_t * child;
    cleri_node_t * fnname = nd->children->node;
    register intptr_t nargs = 0;
    register size_t n = fnname->len;
    register uint32_t key = qbind__hash(fnname->str, n);
    register qbind__fmap_t * fmap = key <= MAX_HASH_VALUE
            ? qbind__fn_map[key]
            : NULL;
    register uint8_t fmflags = (
            fmap &&
            fmap->n == n &&
            ((FN__FLAG_ROOT|FN__FLAG_CHAIN) & flags & fmap->flags) &&
            memcmp(fnname->str, fmap->name, n) == 0) ? fmap->flags : 0;

    nd->data = fmflags ? fmap->fn : NULL;

    /* may set event flag */
    q->flags |= (
        ((FN__FLAG_EV_T|FN__FLAG_EV_C) & q->flags & fmflags) &&
        ((~fmflags & FN__FLAG_EXCL_VAR) || (~flags & FN__FLAG_EXCL_VAR)) &&
        ((~fmflags & FN__FLAG_XROOT) || (~flags & FN__FLAG_ROOT))
    ) << TI_QBIND_BIT_EVENT;

    /* update the value cache if no build-in function is found */
    q->immutable_n += nd->data == NULL;

    /* only used when no build-in function is found */
    fnname->data = NULL;

    /* list (arguments) */
    nd = nd->children->next->node->children->next->node;

    if (fmflags & FN__FLAG_FUT)
    {
        child = nd->children;
        if  (child)
        {
            qbind__peek_statement_for_closure(q, child->node);
            nargs = 1;
            child = child->next ? child->next->next : NULL;
        }
        /* only care about the first argument */
        for(; child; child = child->next ? child->next->next : NULL, ++nargs);
    }
    /* for all other, investigate arguments */
    else for(child = nd->children;
        child;
        child = child->next ? child->next->next : NULL, ++nargs)
    {
        qbind__statement(q, child->node);  /* statement */
    }

    q->flags |= ((fmflags & FN__FLAG_AS_ON_VAR) && 1) << TI_QBIND_BIT_ON_VAR;

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

static inline void qbind__enum(ti_qbind_t * qbind, cleri_node_t * nd)
{
    nd = nd->children->next->node;

    if (nd->cl_obj->gid == CLERI_GID_T_CLOSURE)
    {
        nd->data = NULL;    /* closure */
        ++qbind->immutable_n;
        qbind__statement(qbind, nd->children->next->next->next->node);
    }
}

static void qbind__var_opt_fa(ti_qbind_t * qbind, cleri_node_t * nd)
{
    if (nd->children->next)
    {
        switch (nd->children->next->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            qbind__function(qbind, nd, FN__FLAG_ROOT);
            return;
        case CLERI_GID_ASSIGN:
            qbind__statement(
                    qbind,
                    nd->children->next->node->children->next->node);
            break;
        case CLERI_GID_INSTANCE:
            qbind__thing(qbind, nd->children->next->node);
            return;
        case CLERI_GID_ENUM_:
            qbind__enum(qbind, nd->children->next->node);
            return;
        default:
            assert (0);
            return;
        }
    }
    else
        qbind->flags |= TI_QBIND_FLAG_ON_VAR;

    nd->children->node->data = NULL;
    ++qbind->immutable_n;
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
                    FN__FLAG_CHAIN|qbind__is_onvar(qbind->flags));
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
    ++qbind->immutable_n;
}

static inline void qbind__chain(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_children_t * child = nd->children->next;

    qbind__name_opt_fa(qbind, child->node);

    qbind->flags &= ~TI_QBIND_FLAG_ON_VAR;

    /* index */
    if ((child = child->next)->node->children)
        qbind__index(qbind, child->node);

    /* chain */
    if (child->next)
        qbind__chain(qbind, child->next->node);
}

static void qbind__expr_choice(ti_qbind_t * qbind, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        qbind__chain(qbind, nd);        /* chain */
        return;
    case CLERI_GID_THING_BY_ID:
    {
        intptr_t thing_id = strtoll(nd->str + 1, NULL, 10);
        nd->data = (void *) thing_id;
        return;
    }
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
        ++qbind->immutable_n;
        nd->data = NULL;        /* initialize data to null */
        return;
    case CLERI_GID_TEMPLATE:
    {
        cleri_children_t * child = nd          /* sequence */
                ->children->next->node         /* repeat */
                ->children;

        for (; child; child = child->next)
        {
            if (child->node->cl_obj->tp == CLERI_TP_SEQUENCE)
                qbind__statement(
                        qbind,
                        child->node->children->next->node);
            child->node->data = NULL;
        }

        ++qbind->immutable_n;
        nd->data = NULL;        /* initialize data to null */
        return;
    }
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
    }
}

static inline void qbind__expression(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_node_t * node;
    intptr_t preopr;

    assert (nd->cl_obj->gid == CLERI_GID_EXPRESSION);

    nd->data = ti_do_expression;

    node = nd->children->node;
    preopr = (intptr_t) ti_preopr_bind(node->str, node->len);
    node->data = (void *) preopr;

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
    }
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
 * - count immutable cache requirements
 */
void ti_qbind_probe(ti_qbind_t * qbind, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT ||
            nd->cl_obj->gid == CLERI_GID_STATEMENTS);

    if (nd->cl_obj->gid == CLERI_GID_STATEMENTS)
    {
        for (cleri_children_t * child = nd->children;
             child;
             child = child->next->next)
        {
            qbind__statement(qbind, child->node);   /* statement */

            if (!child->next)
                return;
        }
        return;
    }
    return qbind__statement(qbind, nd);
}
