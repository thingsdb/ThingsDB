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
#include <ti/fn/fnagainat.h>
#include <ti/fn/fnagainin.h>
#include <ti/fn/fnaltraise.h>
#include <ti/fn/fnargs.h>
#include <ti/fn/fnassert.h>
#include <ti/fn/fnassign.h>
#include <ti/fn/fnat.h>
#include <ti/fn/fnbackupinfo.h>
#include <ti/fn/fnbackupsinfo.h>
#include <ti/fn/fnbase64decode.h>
#include <ti/fn/fnbase64encode.h>
#include <ti/fn/fnbool.h>
#include <ti/fn/fnbytes.h>
#include <ti/fn/fncall.h>
#include <ti/fn/fncancel.h>
#include <ti/fn/fnchangeid.h>
#include <ti/fn/fnchoice.h>
#include <ti/fn/fnclear.h>
#include <ti/fn/fnclosure.h>
#include <ti/fn/fncode.h>
#include <ti/fn/fncollectioninfo.h>
#include <ti/fn/fncollectionsinfo.h>
#include <ti/fn/fncontains.h>
#include <ti/fn/fncopy.h>
#include <ti/fn/fncounters.h>
#include <ti/fn/fndatetime.h>
#include <ti/fn/fndeep.h>
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
#include <ti/fn/fndeploymodule.h>
#include <ti/fn/fndoc.h>
#include <ti/fn/fndup.h>
#include <ti/fn/fneach.h>
#include <ti/fn/fnelse.h>
#include <ti/fn/fnemit.h>
#include <ti/fn/fnendswith.h>
#include <ti/fn/fnenum.h>
#include <ti/fn/fnenuminfo.h>
#include <ti/fn/fnenummap.h>
#include <ti/fn/fnenumsinfo.h>
#include <ti/fn/fnequals.h>
#include <ti/fn/fnerr.h>
#include <ti/fn/fnerrors.h>
#include <ti/fn/fnevery.h>
#include <ti/fn/fnexport.h>
#include <ti/fn/fnextend.h>
#include <ti/fn/fnextendunique.h>
#include <ti/fn/fnextract.h>
#include <ti/fn/fnfill.h>
#include <ti/fn/fnfilter.h>
#include <ti/fn/fnfind.h>
#include <ti/fn/fnfindindex.h>
#include <ti/fn/fnfirst.h>
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
#include <ti/fn/fnisfuture.h>
#include <ti/fn/fnisinf.h>
#include <ti/fn/fnisint.h>
#include <ti/fn/fnislist.h>
#include <ti/fn/fnismpdata.h>
#include <ti/fn/fnisnan.h>
#include <ti/fn/fnisnil.h>
#include <ti/fn/fnisraw.h>
#include <ti/fn/fnisregex.h>
#include <ti/fn/fnisroom.h>
#include <ti/fn/fnisset.h>
#include <ti/fn/fnisstr.h>
#include <ti/fn/fnistask.h>
#include <ti/fn/fnisthing.h>
#include <ti/fn/fnistimeval.h>
#include <ti/fn/fnistimezone.h>
#include <ti/fn/fnistuple.h>
#include <ti/fn/fnisunique.h>
#include <ti/fn/fnisutf8.h>
#include <ti/fn/fnjoin.h>
#include <ti/fn/fnjsondump.h>
#include <ti/fn/fnjsonload.h>
#include <ti/fn/fnkeys.h>
#include <ti/fn/fnlast.h>
#include <ti/fn/fnlen.h>
#include <ti/fn/fnlist.h>
#include <ti/fn/fnload.h>
#include <ti/fn/fnlog.h>
#include <ti/fn/fnlower.h>
#include <ti/fn/fnmap.h>
#include <ti/fn/fnmapid.h>
#include <ti/fn/fnmaptype.h>
#include <ti/fn/fnmapwrap.h>
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
#include <ti/fn/fnnse.h>
#include <ti/fn/fnone.h>
#include <ti/fn/fnowner.h>
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
#include <ti/fn/fnrefreshmodule.h>
#include <ti/fn/fnrefs.h>
#include <ti/fn/fnregex.h>
#include <ti/fn/fnremove.h>
#include <ti/fn/fnren.h>
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
#include <ti/fn/fnrestrict.h>
#include <ti/fn/fnrestriction.h>
#include <ti/fn/fnreverse.h>
#include <ti/fn/fnrevoke.h>
#include <ti/fn/fnroom.h>
#include <ti/fn/fnrun.h>
#include <ti/fn/fnsearch.h>
#include <ti/fn/fnset.h>
#include <ti/fn/fnsetargs.h>
#include <ti/fn/fnsetclosure.h>
#include <ti/fn/fnsetdefaultdeep.h>
#include <ti/fn/fnsetenum.h>
#include <ti/fn/fnsetloglevel.h>
#include <ti/fn/fnsetmoduleconf.h>
#include <ti/fn/fnsetmodulescope.h>
#include <ti/fn/fnsetowner.h>
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
#include <ti/fn/fntask.h>
#include <ti/fn/fntasks.h>
#include <ti/fn/fntest.h>
#include <ti/fn/fnthen.h>
#include <ti/fn/fnthing.h>
#include <ti/fn/fntimezonesinfo.h>
#include <ti/fn/fnto.h>
#include <ti/fn/fntothing.h>
#include <ti/fn/fntotype.h>
#include <ti/fn/fntrim.h>
#include <ti/fn/fntrimleft.h>
#include <ti/fn/fntrimright.h>
#include <ti/fn/fntry.h>
#include <ti/fn/fntype.h>
#include <ti/fn/fntypeassert.h>
#include <ti/fn/fntypecount.h>
#include <ti/fn/fntypeinfo.h>
#include <ti/fn/fntypesinfo.h>
#include <ti/fn/fnunique.h>
#include <ti/fn/fnunshift.h>
#include <ti/fn/fnunwrap.h>
#include <ti/fn/fnupper.h>
#include <ti/fn/fnuserinfo.h>
#include <ti/fn/fnusersinfo.h>
#include <ti/fn/fnvalue.h>
#include <ti/fn/fnvalues.h>
#include <ti/fn/fnvmap.h>
#include <ti/fn/fnweek.h>
#include <ti/fn/fnweekday.h>
#include <ti/fn/fnwrap.h>
#include <ti/fn/fnwse.h>
#include <ti/fn/fnyday.h>
#include <ti/fn/fnzone.h>
#include <ti/qbind.h>
#include <ti/preopr.h>

/*
 * Set the `change` flag in case the `TI_QBIND_FLAG_COLLECTION` flag is set.
 *
 * Argument: flags (QBIND flags)
 */
#define qbind__set_collection_soft_change(__f) \
    (__f) |= (((__f) & TI_QBIND_FLAG_COLLECTION) && (~(__f) & TI_QBIND_FLAG_NSE)) << TI_QBIND_BIT_WSE

static void qbind__statement(ti_qbind_t * qbind, cleri_node_t * nd);


/*
 * Used to convert the TI_QBIND_FLAG_ON_VAR to FN__FLAG_XVAR. Therefore
 * the FN__FLAG_XVAR must be `1`.
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
    TOTAL_KEYWORDS = 246,
    MIN_WORD_LENGTH = 2,
    MAX_WORD_LENGTH = 17,
    MIN_HASH_VALUE = 7,
    MAX_HASH_VALUE = 602
};

/*
 * Hash function, created with `pcregrep`. (see documentation above)
 */
static inline unsigned int qbind__hash(
        register const char * s,
        register size_t n)
{
    static unsigned short asso_values[] =
    {
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603,   1, 603,   1, 603,   2, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603,   0, 603,  10,   8,  41,
         21,   2, 117, 232, 120,   0,  11, 241,  14,  36,
         11,  30, 111,   1,   1,   0,   7,  21, 243,  83,
        199, 165,  31, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603, 603, 603, 603, 603,
        603, 603, 603, 603, 603, 603
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
    FN__FLAG_XVAR   = 1<<0, /* must be 1, see qbind__is_onvar() */
    FN__FLAG_EV_T       = 1<<1, /* must be 2, maps to ti_qbind_bit_t */
    FN__FLAG_EV_C       = 1<<2, /* must be 4, maps to ti_qbind_bit_t */
    FN__FLAG_ROOT       = 1<<3,
    FN__FLAG_CHAIN      = 1<<4,
    FN__FLAG_XROOT      = 1<<5, /* exclude creating a change when NOT used as
                                   a chained function. For example `set()` does
                                   not require a change but `.set()` might. */
    FN__FLAG_AS_ON_VAR  = 1<<6, /* handle the function result equal to as
                                   if it was a variable. For example: .get()
                                   might return a pointer to a list which can
                                   NOT be handled as it is a variable but
                                   unwrap() return a thing which could be a
                                   variable as well. */
    FN__FLAG_FUT        = 1<<7, /* must check for closure and do not set wse
                                   by itself */
    FN__FLAG_NSE        = 1<<8, /* set the nse flag */
    FN__FLAG_XNSE       = 1<<9, /* no change when nse flag is set*/
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
 * the return value does not require a change, or if the function itself is
 * already making a change. (then the optimization has no meaning anyway)
 *
 * For example:
 *   t = .my_stored_thing;
 *   t.get('arr').push(42);  // Requires a change, arr is still a pointer.
 *
 *   arr = t.get('arr');
 *   arr.push(42);  // Does not require a change, since arr is assigned and
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
#define CHAIN_CE_X \
        .flags=FN__FLAG_CHAIN|FN__FLAG_EV_C|FN__FLAG_XNSE|FN__FLAG_AS_ON_VAR
#define CHAIN_BE \
        .flags=FN__FLAG_CHAIN|FN__FLAG_EV_C|FN__FLAG_EV_T|FN__FLAG_AS_ON_VAR
#define CHAIN_BE_X \
        .flags=FN__FLAG_CHAIN|FN__FLAG_EV_C|FN__FLAG_EV_T|FN__FLAG_XNSE|FN__FLAG_AS_ON_VAR
#define CHAIN_CE_XX \
        .flags=FN__FLAG_CHAIN|FN__FLAG_XVAR|FN__FLAG_XNSE|FN__FLAG_EV_C|FN__FLAG_AS_ON_VAR
#define CHAIN_FUT \
        .flags=FN__FLAG_CHAIN|FN__FLAG_AS_ON_VAR|FN__FLAG_FUT
#define ROOT_FUT \
        .flags=FN__FLAG_ROOT|FN__FLAG_FUT
#define BOTH_NE \
        .flags=FN__FLAG_ROOT|FN__FLAG_CHAIN|FN__FLAG_AS_ON_VAR
#define BOTH_CE_XXROOT \
        .flags=FN__FLAG_ROOT|FN__FLAG_CHAIN|FN__FLAG_EV_C|FN__FLAG_XROOT|FN__FLAG_XNSE|FN__FLAG_AS_ON_VAR
#define XROOT_NE \
        .flags=FN__FLAG_ROOT
#define XROOT_BE \
        .flags=FN__FLAG_ROOT|FN__FLAG_EV_C|FN__FLAG_EV_T
#define XCHAIN_NE \
        .flags=FN__FLAG_CHAIN
#define XROOT_NSE \
        .flags=FN__FLAG_ROOT|FN__FLAG_NSE

qbind__fmap_t qbind__fn_mapping[TOTAL_KEYWORDS] = {
    {.name="add",               .fn=do__f_add,                  CHAIN_CE_XX},
    {.name="again_at",          .fn=do__f_again_at,             CHAIN_BE},
    {.name="again_in",          .fn=do__f_again_in,             CHAIN_BE},
    {.name="alt_raise",         .fn=do__f_alt_raise,            ROOT_NE},
    {.name="args",              .fn=do__f_args,                 CHAIN_NE},
    {.name="assert_err",        .fn=do__f_assert_err,           ROOT_NE},
    {.name="assert",            .fn=do__f_assert,               ROOT_NE},
    {.name="assign",            .fn=do__f_assign,               CHAIN_CE_X},
    {.name="at",                .fn=do__f_at,                   CHAIN_NE},
    {.name="auth_err",          .fn=do__f_auth_err,             ROOT_NE},
    {.name="backup_info",       .fn=do__f_backup_info,          ROOT_NE},
    {.name="backups_info",      .fn=do__f_backups_info,         ROOT_NE},
    {.name="bad_data_err",      .fn=do__f_bad_data_err,         ROOT_NE},
    {.name="base64_decode",     .fn=do__f_base64_decode,        ROOT_NE},
    {.name="base64_encode",     .fn=do__f_base64_encode,        ROOT_NE},
    {.name="bool",              .fn=do__f_bool,                 ROOT_NE},
    {.name="bytes",             .fn=do__f_bytes,                ROOT_NE},
    {.name="call",              .fn=do__f_call,                 XCHAIN_NE},
    {.name="cancel",            .fn=do__f_cancel,               CHAIN_BE},
    {.name="cancelled_err",     .fn=do__f_cancelled_err,        ROOT_NE},
    {.name="change_id",         .fn=do__f_change_id,            ROOT_NE},
    {.name="choice",            .fn=do__f_choice,               CHAIN_NE},
    {.name="clear",             .fn=do__f_clear,                CHAIN_CE_X},
    {.name="closure",           .fn=do__f_closure,              BOTH_NE},
    {.name="code",              .fn=do__f_code,                 CHAIN_NE},
    {.name="collection_info",   .fn=do__f_collection_info,      ROOT_NE},
    {.name="collections_info",  .fn=do__f_collections_info,     ROOT_NE},
    {.name="contains",          .fn=do__f_contains,             CHAIN_NE},
    {.name="copy",              .fn=do__f_copy,                 CHAIN_NE},
    {.name="counters",          .fn=do__f_counters,             ROOT_NE},
    {.name="datetime",          .fn=do__f_datetime,             ROOT_NE},
    {.name="deep",              .fn=do__f_deep,                 ROOT_NE},
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
    {.name="del",               .fn=do__f_del,                  CHAIN_BE_X},
    {.name="deploy_module",     .fn=do__f_deploy_module,        ROOT_TE},
    {.name="doc",               .fn=do__f_doc,                  CHAIN_NE},
    {.name="dup",               .fn=do__f_dup,                  CHAIN_NE},
    {.name="each",              .fn=do__f_each,                 CHAIN_NE},
    {.name="else",              .fn=do__f_else,                 CHAIN_FUT},
    {.name="emit",              .fn=do__f_emit,                 CHAIN_NE},
    {.name="ends_with",         .fn=do__f_ends_with,            CHAIN_NE},
    {.name="enum_info",         .fn=do__f_enum_info,            ROOT_NE},
    {.name="enum_map",          .fn=do__f_enum_map,             ROOT_NE},
    {.name="enum",              .fn=do__f_enum,                 ROOT_NE},
    {.name="enums_info",        .fn=do__f_enums_info,           ROOT_NE},
    {.name="equals",            .fn=do__f_equals,               CHAIN_NE},
    {.name="err",               .fn=do__f_err,                  BOTH_NE},
    {.name="every",             .fn=do__f_every,                CHAIN_NE},
    {.name="export",            .fn=do__f_export,               ROOT_NE},
    {.name="extend_unique",     .fn=do__f_extend_unique,        CHAIN_CE_XX},
    {.name="extend",            .fn=do__f_extend,               CHAIN_CE_XX},
    {.name="extract",           .fn=do__f_extract,              CHAIN_NE},
    {.name="fill",              .fn=do__f_fill,                 CHAIN_CE_XX},
    {.name="filter",            .fn=do__f_filter,               CHAIN_NE},
    {.name="find_index",        .fn=do__f_find_index,           CHAIN_NE},
    {.name="find",              .fn=do__f_find,                 XCHAIN_NE},
    {.name="first",             .fn=do__f_first,                CHAIN_NE},
    {.name="float",             .fn=do__f_float,                ROOT_NE},
    {.name="forbidden_err",     .fn=do__f_forbidden_err,        ROOT_NE},
    {.name="format",            .fn=do__f_format,               CHAIN_NE},
    {.name="future",            .fn=do__f_future,               ROOT_FUT},
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
    {.name="is_future",         .fn=do__f_is_future,            ROOT_NE},
    {.name="is_inf",            .fn=do__f_is_inf,               ROOT_NE},
    {.name="is_int",            .fn=do__f_is_int,               ROOT_NE},
    {.name="is_list",           .fn=do__f_is_list,              ROOT_NE},
    {.name="is_mpdata",         .fn=do__f_is_mpdata,            ROOT_NE},
    {.name="is_nan",            .fn=do__f_is_nan,               ROOT_NE},
    {.name="is_nil",            .fn=do__f_is_nil,               ROOT_NE},
    {.name="is_raw",            .fn=do__f_is_raw,               ROOT_NE},
    {.name="is_regex",          .fn=do__f_is_regex,             ROOT_NE},
    {.name="is_room",           .fn=do__f_is_room,              ROOT_NE},
    {.name="is_set",            .fn=do__f_is_set,               ROOT_NE},
    {.name="is_str",            .fn=do__f_is_str,               ROOT_NE},
    {.name="is_task",           .fn=do__f_is_task,              ROOT_NE},
    {.name="is_thing",          .fn=do__f_is_thing,             ROOT_NE},
    {.name="is_time_zone",      .fn=do__f_is_time_zone,         ROOT_NE},
    {.name="is_timeval",        .fn=do__f_is_timeval,           ROOT_NE},
    {.name="is_tuple",          .fn=do__f_is_tuple,             ROOT_NE},
    {.name="is_unique",         .fn=do__f_is_unique,            CHAIN_NE},
    {.name="is_utf8",           .fn=do__f_is_utf8,              ROOT_NE},
    {.name="join",              .fn=do__f_join,                 CHAIN_NE},
    {.name="json_dump",         .fn=do__f_json_dump,            ROOT_NE},
    {.name="json_load",         .fn=do__f_json_load,            ROOT_NE},
    {.name="keys",              .fn=do__f_keys,                 CHAIN_NE},
    {.name="last",              .fn=do__f_last,                 CHAIN_NE},
    {.name="len",               .fn=do__f_len,                  CHAIN_NE},
    {.name="list",              .fn=do__f_list,                 ROOT_NE},
    {.name="load",              .fn=do__f_load,                 CHAIN_NE},
    {.name="log",               .fn=do__f_log,                  ROOT_NE},
    {.name="lookup_err",        .fn=do__f_lookup_err,           ROOT_NE},
    {.name="lower",             .fn=do__f_lower,                CHAIN_NE},
    {.name="map",               .fn=do__f_map,                  CHAIN_NE},
    {.name="map_id",            .fn=do__f_map_id,               CHAIN_NE},
    {.name="map_type",          .fn=do__f_map_type,             CHAIN_NE},
    {.name="map_wrap",          .fn=do__f_map_wrap,             CHAIN_NE},
    {.name="max_quota_err",     .fn=do__f_max_quota_err,        ROOT_NE},
    {.name="mod_enum",          .fn=do__f_mod_enum,             ROOT_CE},
    {.name="mod_type",          .fn=do__f_mod_type,             ROOT_CE},
    {.name="module_info",       .fn=do__f_module_info,          ROOT_NE},
    {.name="modules_info",      .fn=do__f_modules_info,         ROOT_NE},
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
    {.name="nse",               .fn=do__f_nse,                  XROOT_NSE},
    {.name="node_err",          .fn=do__f_node_err,             ROOT_NE},
    {.name="node_info",         .fn=do__f_node_info,            ROOT_NE},
    {.name="nodes_info",        .fn=do__f_nodes_info,           ROOT_NE},
    {.name="now",               .fn=do__f_now,                  ROOT_NE},
    {.name="num_arguments_err", .fn=do__f_num_arguments_err,    ROOT_NE},
    {.name="one",               .fn=do__f_one,                  CHAIN_NE},
    {.name="operation_err",     .fn=do__f_operation_err,        ROOT_NE},
    {.name="overflow_err",      .fn=do__f_overflow_err,         ROOT_NE},
    {.name="owner",             .fn=do__f_owner,                CHAIN_NE},
    {.name="pop",               .fn=do__f_pop,                  CHAIN_CE_XX},
    {.name="procedure_doc",     .fn=do__f_procedure_doc,        ROOT_NE},
    {.name="procedure_info",    .fn=do__f_procedure_info,       ROOT_NE},
    {.name="procedures_info",   .fn=do__f_procedures_info,      ROOT_NE},
    {.name="push",              .fn=do__f_push,                 CHAIN_CE_XX},
    {.name="raise",             .fn=do__f_raise,                ROOT_NE},
    {.name="rand",              .fn=do__f_rand,                 ROOT_NE},
    {.name="randint",           .fn=do__f_randint,              ROOT_NE},
    {.name="randstr",           .fn=do__f_randstr,              ROOT_NE},
    {.name="range",             .fn=do__f_range,                ROOT_NE},
    {.name="reduce",            .fn=do__f_reduce,               XCHAIN_NE},
    {.name="refresh_module",    .fn=do__f_refresh_module,       ROOT_TE},
    {.name="refs",              .fn=do__f_refs,                 ROOT_NE},
    {.name="regex",             .fn=do__f_regex,                ROOT_NE},
    {.name="remove",            .fn=do__f_remove,               CHAIN_CE_X},
    {.name="ren",               .fn=do__f_ren,                  CHAIN_CE_X},
    {.name="rename_collection", .fn=do__f_rename_collection,    ROOT_TE},
    {.name="rename_enum",       .fn=do__f_rename_enum,          ROOT_CE},
    {.name="rename_module",     .fn=do__f_rename_module,        ROOT_TE},
    {.name="rename_procedure",  .fn=do__f_rename_procedure,     ROOT_BE},
    {.name="rename_type",       .fn=do__f_rename_type,          ROOT_CE},
    {.name="rename_user",       .fn=do__f_rename_user,          ROOT_TE},
    {.name="replace",           .fn=do__f_replace,              CHAIN_NE},
    {.name="reset_counters",    .fn=do__f_reset_counters,       ROOT_NE},
    {.name="restart_module",    .fn=do__f_restart_module,       ROOT_NE},
    {.name="restore",           .fn=do__f_restore,              ROOT_TE},
    {.name="restrict",          .fn=do__f_restrict,             CHAIN_CE_X},
    {.name="restriction",       .fn=do__f_restriction,          CHAIN_NE},
    {.name="reverse",           .fn=do__f_reverse,              CHAIN_NE},
    {.name="revoke",            .fn=do__f_revoke,               ROOT_TE},
    {.name="room",              .fn=do__f_room,                 ROOT_NE},
    {.name="run",               .fn=do__f_run,                  XROOT_NE},
    {.name="search",            .fn=do__f_search,               CHAIN_NE},
    {.name="set_args",          .fn=do__f_set_args,             CHAIN_BE},
    {.name="set_closure",       .fn=do__f_set_closure,          CHAIN_BE},
    {.name="set_default_deep",  .fn=do__f_set_default_deep,     ROOT_TE},
    {.name="set_enum",          .fn=do__f_set_enum,             ROOT_CE},
    {.name="set_log_level",     .fn=do__f_set_log_level,        ROOT_NE},
    {.name="set_module_conf",   .fn=do__f_set_module_conf,      ROOT_TE},
    {.name="set_module_scope",  .fn=do__f_set_module_scope,     ROOT_TE},
    {.name="set_owner",         .fn=do__f_set_owner,            CHAIN_BE},
    {.name="set_password",      .fn=do__f_set_password,         ROOT_TE},
    {.name="set_time_zone",     .fn=do__f_set_time_zone,        ROOT_TE},
    {.name="set_type",          .fn=do__f_set_type,             ROOT_CE},
    {.name="set",               .fn=do__f_set,                  BOTH_CE_XXROOT},
    {.name="shift",             .fn=do__f_shift,                CHAIN_CE_XX},
    {.name="shutdown",          .fn=do__f_shutdown,             ROOT_NE},
    {.name="some",              .fn=do__f_some,                 CHAIN_NE},
    {.name="sort",              .fn=do__f_sort,                 CHAIN_NE},
    {.name="splice",            .fn=do__f_splice,               CHAIN_CE_XX},
    {.name="split",             .fn=do__f_split,                CHAIN_NE},
    {.name="starts_with",       .fn=do__f_starts_with,          CHAIN_NE},
    {.name="str",               .fn=do__f_str,                  ROOT_NE},
    {.name="syntax_err",        .fn=do__f_syntax_err,           ROOT_NE},
    {.name="test",              .fn=do__f_test,                 CHAIN_NE},
    {.name="task",              .fn=do__f_task,                 ROOT_BE},
    {.name="tasks",             .fn=do__f_tasks,                ROOT_NE},
    {.name="then",              .fn=do__f_then,                 CHAIN_FUT},
    {.name="thing",             .fn=do__f_thing,                ROOT_NE},
    {.name="time_zones_info",   .fn=do__f_time_zones_info,      ROOT_NE},
    {.name="timeval",           .fn=do__f_timeval,              ROOT_NE},
    {.name="to_thing",          .fn=do__f_to_thing,             CHAIN_CE_X},
    {.name="to_type",           .fn=do__f_to_type,              CHAIN_CE_X},
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
    {.name="unique",            .fn=do__f_unique,               CHAIN_NE},
    {.name="unshift",           .fn=do__f_unshift,              CHAIN_CE_XX},
    {.name="unwrap",            .fn=do__f_unwrap,               CHAIN_NE},
    {.name="upper",             .fn=do__f_upper,                CHAIN_NE},
    {.name="user_info",         .fn=do__f_user_info,            ROOT_NE},
    {.name="users_info",        .fn=do__f_users_info,           ROOT_NE},
    {.name="value_err",         .fn=do__f_value_err,            ROOT_NE},
    {.name="value",             .fn=do__f_value,                CHAIN_NE},
    {.name="values",            .fn=do__f_values,               CHAIN_NE},
    {.name="vmap",              .fn=do__f_vmap,                 CHAIN_NE},
    {.name="week",              .fn=do__f_week,                 CHAIN_NE},
    {.name="weekday",           .fn=do__f_weekday,              CHAIN_NE},
    {.name="wrap",              .fn=do__f_wrap,                 CHAIN_NE},
    {.name="wse",               .fn=do__f_wse,                  XROOT_BE},
    {.name="yday",              .fn=do__f_yday,                 CHAIN_NE},
    {.name="zero_div_err",      .fn=do__f_zero_div_err,         ROOT_NE},
    {.name="zone",              .fn=do__f_zone,                 CHAIN_NE},
};

static qbind__fmap_t * qbind__fn_map[MAX_HASH_VALUE+1];

/*
 * Initializes ThingsDB function mapping.
 *
 * Note: must be called exactly once when starting ThingsDB.
 */
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

/*
 * Used to swap nodes in the correct order, for example:
 *   1 + 2 * 3
 * We first need to calculate 2 * 3 and then add the one.
 *
 * Note: This function *only* swaps nodes and does no other statement
 *       work because the function may visit a node multiple times since it
 *       may be called recursive. *
 */
static _Bool qbind__swap(cleri_node_t ** parent, uint32_t parent_gid)
{
    uint32_t gid = (*parent)->children->next->cl_obj->gid;
    cleri_node_t * childb = (*parent)->children->next->next;

    if (childb->children->cl_obj->gid == CLERI_GID_OPERATIONS &&
        qbind__swap(&childb->children, gid))
    {
        /* Swap operations */
        cleri_node_t ** syntax_childa;
        cleri_node_t * tmp = *parent;  /* operations */
        *parent = childb->children;  /* operations */

        gid = (*parent)->children->next->cl_obj->gid;
        syntax_childa = &(*parent)->children->children;
        childb->children = *syntax_childa;
        *syntax_childa = tmp;

        /* Recursive swapping */
        qbind__swap(syntax_childa, gid);
    }

    /* Ternary operations require handing from right-to-left, whereas the other
     * operations must be handled from left-to-right. # bug #271
     */
    return gid > parent_gid || (
            gid == parent_gid && gid != CLERI_GID_OPR8_TERNARY);
}

/*
 * Analyze operations nodes.
 *
 * This function may swap some nodes in case this is required. For example,
 * multiplication must be handled before adding etc.
 *
 * The language uses keys like:
 *  - CLERI_GID_OPR0_MUL_DIV_MOD
 *  - CLERI_GID_OPR1_ADD_SUB
 *  - CLERI_GID_OPR2_BITWISE_AND,
 *  - etc....
 *  The keys are numbered so the corresponding ID's can be used as order.
 */
static _Bool qbind__operations(
        ti_qbind_t * qbind,
        cleri_node_t ** parent,
        uint32_t parent_gid)
{
    static const ti_do_cb operation_cb[9] = {
            ti_do_operation,    /* CLERI_GID_OPR0_MUL_DIV_MOD */
            ti_do_operation,    /* CLERI_GID_OPR1_ADD_SUB */
            ti_do_operation,    /* CLERI_GID_OPR2_BITWISE_AND */
            ti_do_operation,    /* CLERI_GID_OPR3_BITWISE_XOR */
            ti_do_operation,    /* CLERI_GID_OPR4_BITWISE_OR */
            ti_do_operation,    /* CLERI_GID_OPR5_COMPARE */
            ti_do_compare_and,  /* CLERI_GID_OPR6_CMP_AND */
            ti_do_compare_or,   /* CLERI_GID_OPR7_CMP_OR */
            ti_do_ternary,      /* CLERI_GID_OPR8_TERNARY */
    };
    uint32_t gid = (*parent)->children->next->cl_obj->gid;
    cleri_node_t * childb = (*parent)->children->next->next;

    (*parent)->data = operation_cb[gid - CLERI_GID_OPR0_MUL_DIV_MOD];

    assert (gid >= CLERI_GID_OPR0_MUL_DIV_MOD &&
            gid <= CLERI_GID_OPR8_TERNARY);

    qbind__statement(qbind, (*parent)->children);

    if (gid == CLERI_GID_OPR8_TERNARY)
        qbind__statement(
                qbind,
                (*parent)->children->next->children->next);

    if (childb->children->cl_obj->gid != CLERI_GID_OPERATIONS)
    {
        qbind__statement(qbind, childb);
    }
    else if (qbind__operations(qbind, &childb->children, gid))
    {
        /* Swap operations */
        cleri_node_t ** syntax_childa;
        cleri_node_t * tmp = (*parent);  /* operations */
        *parent = childb->children;  /* operations */

        gid = (*parent)->children->next->cl_obj->gid;

        assert (gid >= CLERI_GID_OPR0_MUL_DIV_MOD &&
                gid <= CLERI_GID_OPR8_TERNARY);

        syntax_childa = &(*parent)->children->children;
        childb->children = *syntax_childa;
        *syntax_childa = tmp;

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

    /* Ternary operations require handing from right-to-left, whereas the other
     * operations must be handled from left-to-right. # bug #271
     */
    return gid > parent_gid || (
            gid == parent_gid && gid != CLERI_GID_OPR8_TERNARY);
}

/*
 * Peek for a closure and if found, analyze the statement but ignore any wse
 * flags.
 *
 * Only called in relation to a future (future,then,else).
 * If the argument is a closure, this closure be called with it's own query
 * and change (if required).
 */
static void qbind__peek_statement_for_closure(
        ti_qbind_t * q,
        cleri_node_t * nd)
{
    cleri_node_t * node;

    if ((node = nd->children)->cl_obj->gid == CLERI_GID_CLOSURE)
    {
        uint8_t no_wse_flag = ~q->flags & TI_QBIND_FLAG_WSE;
        qbind__statement(q, nd);
        q->flags &= ~no_wse_flag;
        return;
    }

    qbind__statement(q, nd);  /* statement */
}

/*
 * Analyze a function call.
 *   - Arguments will be analyzed
 *   - Based on the function, the wse flag will be set
 *   - Future (then,else) will be peeked
 *   - Immutable counter will increase for no-build-in functions
 */
static void qbind__function(
        ti_qbind_t * q,
        cleri_node_t * nd,
        int flags)
{
    cleri_node_t * child;
    cleri_node_t * fnname = nd->children;
    register intptr_t nargs = 0;
    register size_t n = fnname->len;
    register uint32_t key = qbind__hash(fnname->str, n);
    register qbind__fmap_t * fmap = key <= MAX_HASH_VALUE
            ? qbind__fn_map[key]
            : NULL;
    register qbind__fn_flag_t fmflags = (
            fmap &&
            fmap->n == n &&
            ((FN__FLAG_ROOT|FN__FLAG_CHAIN) & flags & fmap->flags) &&
            memcmp(fnname->str, fmap->name, n) == 0) ? fmap->flags : 0;

    nd->data = fmflags ? fmap->fn : NULL;

    /* may set wse flag */
    q->flags |= (
        ((FN__FLAG_EV_T|FN__FLAG_EV_C) & q->flags & fmflags) &&
        ((~fmflags & FN__FLAG_XVAR) || (~flags & FN__FLAG_XVAR)) &&
        ((~fmflags & FN__FLAG_XROOT) || (~flags & FN__FLAG_ROOT)) &&
        ((~fmflags & FN__FLAG_XNSE) || (~q->flags & TI_QBIND_FLAG_NSE))
    ) << TI_QBIND_BIT_WSE;

    /* update the value cache if no build-in function is found */
    q->immutable_n += nd->data == NULL;

    /* only used when no build-in function is found */
    fnname->data = NULL;

    /* list (arguments) */
    nd = nd->children->next->children->next;

    if (fmflags & FN__FLAG_NSE)
        q->flags |= TI_QBIND_FLAG_NSE;

    if (fmflags & FN__FLAG_FUT)
    {
        child = nd->children;
        if  (child)
        {
            qbind__peek_statement_for_closure(q, child);
            nargs = 1;
            child = child->next ? child->next->next : NULL;
        }
        /* only care about the first argument */
        for(; child; child = child->next ? child->next->next : NULL, ++nargs)
        {
            qbind__statement(q, child);  /* statement */
        }
    }
    /* for all other, investigate arguments */
    else for(child = nd->children;
        child;
        child = child->next ? child->next->next : NULL, ++nargs)
    {
        qbind__statement(q, child);  /* statement */
    }

    q->flags |= ((fmflags & FN__FLAG_AS_ON_VAR) && 1) << TI_QBIND_BIT_ON_VAR;

    /* bind `nargs` to node->data */
    nd->data = (void *) nargs;
}

/*
 * Analyze indexes, for example:
 *    array[0];
 *
 * Note: This function also is capable of analyzing slices like:
 *    array[0:10:2]
 */
static void qbind__index(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_node_t * child = nd->children;
    assert (child);
    do
    {
        cleri_node_t * c = child        /* sequence */
                ->children->next        /* slice */
                ->children;

        if (child->children->next->next->next)
        {
            qbind__set_collection_soft_change(qbind->flags);
            qbind__statement(qbind, child             /* sequence */
                    ->children->next->next->next      /* assignment */
                    ->children->next);                /* statement */
        }

        for (; c; c = c->next)
            if (c->cl_obj->gid == CLERI_GID_STATEMENT)
                qbind__statement(qbind, c);
    }
    while ((child = child->next));
}

/*
 * Analyze a thing.
 *
 * Note: a thing consists of keys and values, where each value is a statement
 *       which will be analyzed as well.
 */
static inline void qbind__thing(ti_qbind_t * qbind, cleri_node_t * nd)
{
    uintptr_t sz = 0;
    cleri_node_t * child = nd           /* sequence */
            ->children->next            /* list */
            ->children;
    for (; child; child = child->next->next)
    {
        /* sequence(name: statement) (only investigate the statements */
        if (child->children->next->next == NULL)
        {
            child->children->data = NULL;
            ++qbind->immutable_n;
        }
        else
            qbind__statement(
                    qbind,
                    child->children->next->next);  /* statement */
        ++sz;
        if (!child->next)
            break;
    }
    nd->data = (void *) sz;
}

static inline void qbind__closure(ti_qbind_t * qbind, cleri_node_t * nd)
{
    intptr_t closure_wse;
    uint8_t flags = qbind->flags & (TI_QBIND_FLAG_FOR_LOOP|TI_QBIND_FLAG_WSE);

    nd->data = ti_do_closure;
    nd->children->data = NULL;

    qbind->flags &= ~(TI_QBIND_FLAG_FOR_LOOP|TI_QBIND_FLAG_WSE);

    /* investigate the statement, the rest can be skipped */
    qbind__statement(
            qbind,
            nd->children->next->next->next);

    closure_wse = (qbind->flags & TI_QBIND_FLAG_WSE) ? TI_CLOSURE_FLAG_WSE : 0;
    nd->children->next->data = (void *) closure_wse;

    ++qbind->immutable_n;
    qbind->flags |= flags;
}

/*
 * Analyze enumerator.
 *
 * This function only does work in case a closure is used within the enumerator.
 *
 * For example:
 *   MyEnum{||x > y ? 'XKEY' : 'YKEY'}
 *
 * When an enum is used as a function call, this function will not be used and
 * no analyzing is required in case of a fixed key like:
 *   MyEnum{KEY}
 */
static inline void qbind__enum(ti_qbind_t * qbind, cleri_node_t * nd)
{
    if ((nd = nd->children->next)->cl_obj->gid == CLERI_GID_CLOSURE)
        qbind__closure(qbind, nd);
}

/*
 * Used to make the correct call and increase the immutable counter if required.
 *
 * Options are:
 *   - some_function_call(...)
 *   - my_assignmend = ...
 *   - MyInstance{...}
 *   - MyEnum{...}
 *   - some_variable...
 */
static void qbind__var_opt_fa(ti_qbind_t * qbind, cleri_node_t * nd)
{
    if (nd->children->next)
    {
        switch (nd->children->next->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            qbind__function(qbind, nd, FN__FLAG_ROOT);
            return;
        case CLERI_GID_ASSIGN:
            qbind__statement(
                    qbind,
                    nd->children->next->children->next);
            break;
        case CLERI_GID_INSTANCE:
            qbind__thing(qbind, nd->children->next);
            return;
        case CLERI_GID_ENUM_:
            qbind__enum(qbind, nd->children->next);
            return;
        default:
            assert (0);
            return;
        }
    }
    else
        qbind->flags |= TI_QBIND_FLAG_ON_VAR;

    nd->children->data = NULL;
    ++qbind->immutable_n;
}

/*
 * Used to make the correct call when chained and increase immutable counter
 * ir required.
 *
 * Options are:
 * - .my_function_call(...)
 * - .my_prop = ...
 * - .my_prop
 */
static void qbind__name_opt_fa(ti_qbind_t * qbind, cleri_node_t * nd)
{
    if (nd->children->next)
    {
        switch (nd->children->next->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            qbind__function(
                    qbind,
                    nd,
                    FN__FLAG_CHAIN|qbind__is_onvar(qbind->flags));
            return;
        case CLERI_GID_ASSIGN:
            qbind__set_collection_soft_change(qbind->flags);
            qbind__statement(
                    qbind,
                    nd->children->next->children->next);
            break;
        default:
            assert (0);
            return;
        }
    }
    nd->children->data = NULL;
    ++qbind->immutable_n;
}

/*
 * Analyze a chain (when a dot `.` is used), Examples:
 *
 * .something
 * .something[...]
 * .something.other();
 */
static inline void qbind__chain(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_node_t * child = nd->children->next;

    qbind__name_opt_fa(qbind, child);

    qbind->flags &= ~TI_QBIND_FLAG_ON_VAR;

    /* index */
    if ((child = child->next)->children)
        qbind__index(qbind, child);

    /* chain */
    if (child->next)
        qbind__chain(qbind, child->next);
}

/*
 * Analyze a part of a statement.
 *
 * This function analyzes things, enumerators, immutable values and more.
 */
static void qbind__expr_choice(ti_qbind_t * qbind, cleri_node_t * nd)
{
    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        qbind__chain(qbind, nd);        /* chain */
        return;
    case CLERI_GID_T_INT:
    case CLERI_GID_T_FLOAT:
    case CLERI_GID_T_STRING:
    case CLERI_GID_T_REGEX:
        ++qbind->immutable_n;
        nd->data = NULL;        /* initialize data to null */
        return;
    case CLERI_GID_TEMPLATE:
    {
        cleri_node_t * child = nd           /* sequence */
                ->children->next            /* repeat */
                ->children;

        for (; child; child = child->next)
        {
            if (child->cl_obj->tp == CLERI_TP_SEQUENCE)
                qbind__statement(
                        qbind,
                        child->children->next);
            child->data = NULL;
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
        cleri_node_t * child = nd       /* sequence */
                ->children->next        /* list */
                ->children;
        for (; child; child = child->next ? child->next->next : NULL, ++sz)
            qbind__statement(qbind, child);  /* statement */
        nd->data = (void *) sz;
        qbind->flags |= TI_QBIND_FLAG_ON_VAR;   /* enable var mode */
        return;
    }
    case CLERI_GID_PARENTHESIS:
        qbind__statement(qbind, nd->children->next);
    }
}

/*
 * Analyze an expression. An expression may start with some +, - or ! signs,
 * followed by a function, variable or something else, next an optional index
 * and an optional chain. Examples:
 *
 * !!convert_to_bool;
 * -x;
 * my_var[idx].func();
 */
static inline void qbind__expression(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_node_t * node;
    intptr_t preopr;

    assert (nd->cl_obj->gid == CLERI_GID_EXPRESSION);

    nd->data = ti_do_expression;

    node = nd->children;
    preopr = (intptr_t) ti_preopr_bind(node->str, node->len);
    node->data = (void *) preopr;

    qbind__expr_choice(qbind, nd->children->next);

    /* index */
    if (nd->children->next->next->children)
        qbind__index(qbind, nd->children->next->next);

    /* chain */
    if (nd->children->next->next->next)
        qbind__chain(qbind, nd->children->next->next->next);
}

static inline void qbind__if_statement(ti_qbind_t * qbind, cleri_node_t * nd)
{
    qbind__statement(qbind, nd->children->next->next);

    /* set true node */
    nd->children->data = nd->children->next->next->next->next;

    qbind__statement(qbind, nd->children->data);

    /* set else node */
    nd->children->next->data = nd->children->next->next->next->next->next
        ? nd->children->next->next->next->next->next->children->next
        : NULL;

    if (nd->children->next->data)
        qbind__statement(qbind, nd->children->next->data);

    nd->data = ti_do_if_statement;
}

static inline void qbind__return_statement(
        ti_qbind_t * qbind,
        cleri_node_t * nd)
{
    cleri_node_t * node = nd->children->next->children;
    qbind__statement(qbind, node);
    if (node->next)
    {
        node = nd->children->data = node->next->next;
        qbind__statement(qbind, node);
        if (node->next)
        {
            qbind__statement(qbind, node->next->next);
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

static inline void qbind__for_statement(ti_qbind_t * q, cleri_node_t * nd)
{
    register uint8_t no_for_loop = ~q->flags & TI_QBIND_FLAG_FOR_LOOP;
    cleri_node_t * tmp, * child = nd->
            children->              /* for  */
            next->                  /* (    */
            next;                   /* List(variable) */

    nd->data = ti_do_for_loop;

    /* count number of arguments (variable) */
    nd = child;
    for(tmp = nd->children;
        tmp;
        tmp = tmp->next ? tmp->next->next : NULL, ++q->immutable_n)
        tmp->data = NULL;

    qbind__statement(q, (child = child->next->next));
    q->flags |= TI_QBIND_FLAG_FOR_LOOP;
    qbind__statement(q, (child = child->next->next));
    q->flags &= ~no_for_loop;
}

static inline void qbind__block(ti_qbind_t * qbind, cleri_node_t * nd)
{
    cleri_node_t * child = nd       /* seq<{, comment, list, }> */
            ->children->next->next  /* list statements */
            ->children;             /* first child, not empty */

    nd->data = ti_do_block;

    do
    {
        qbind__statement(qbind, child);  /* statement */
        if (!child->next)
            break;
    }
    while ((child = child->next->next));

    return;
}


/*
 * Entry point for analyzing a statement.
 *
 * Almost anything in the grammar may call this function since statements
 * can exist on may places in the ThingsDB language.
 */
static void qbind__statement(ti_qbind_t * qbind, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT);

    switch (nd->children->cl_obj->gid)
    {
    case CLERI_GID_IF_STATEMENT:
        qbind__if_statement(qbind, nd->children);
        return;
    case CLERI_GID_RETURN_STATEMENT:
        qbind__return_statement(qbind, nd->children);
        return;
    case CLERI_GID_FOR_STATEMENT:
        qbind__for_statement(qbind, nd->children);
        return;
    case CLERI_GID_K_CONTINUE:
        if (~qbind->flags & TI_QBIND_FLAG_FOR_LOOP)
            qbind->flags |= TI_QBIND_FLAG_ILL_CONTINUE;
        nd->children->data = ti_do_continue;
        return;
    case CLERI_GID_K_BREAK:
        if (~qbind->flags & TI_QBIND_FLAG_FOR_LOOP)
            qbind->flags |= TI_QBIND_FLAG_ILL_BREAK;
        nd->children->data = ti_do_break;
        return;
    case CLERI_GID_CLOSURE:
        qbind__closure(qbind, nd->children);
        return;
    case CLERI_GID_EXPRESSION:
        qbind->flags &= ~TI_QBIND_FLAG_ON_VAR;
        qbind__expression(qbind, nd->children);
        return;
    case CLERI_GID_BLOCK:
        qbind__block(qbind, nd->children);
        return;
    case CLERI_GID_OPERATIONS:
        qbind__operations(qbind, &nd->children, 0);
    }
}

/*
 * Investigates the following:
 *
 * - array sizes (stored in node->data)
 * - number of function arguments (stored in node->data)
 * - function bindings (stored in node->data)
 * - change requirement (set as qbind wse flag)
 * - closure detection and change requirement (set as flag to node->data)
 * - re-arrange operations to honor compare order
 * - count immutable cache requirements
 */
void ti_qbind_probe(ti_qbind_t * qbind, cleri_node_t * nd)
{
    assert (nd->cl_obj->gid == CLERI_GID_STATEMENT ||
            nd->cl_obj->gid == CLERI_GID_STATEMENTS);

    if (nd->cl_obj->gid == CLERI_GID_STATEMENTS)
    {
        for (nd = nd->children;
             nd;
             nd = nd->next->next)
        {
            qbind__statement(qbind, nd);   /* statement */

            if (!nd->next)
                return;
        }
        return;
    }
    return qbind__statement(qbind, nd);
}
