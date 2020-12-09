/*
 * ti/val.c
 */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/closure.h>
#include <ti/datetime.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/member.h>
#include <ti/member.inline.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/raw.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
#include <ti/scope.h>
#include <ti/template.h>
#include <ti/thing.inline.h>
#include <ti/things.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/verror.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/wrap.inline.h>
#include <tiinc.h>
#include <util/logger.h>
#include <util/strx.h>

#define VAL__CMP(__s) ti_raw_eq_strn((*(ti_raw_t **) val), __s, strlen(__s))

static ti_val_t * val__empty_bin;
static ti_val_t * val__empty_str;
static ti_val_t * val__sany;
static ti_val_t * val__snil;
static ti_val_t * val__strue;
static ti_val_t * val__sfalse;
static ti_val_t * val__sbool;
static ti_val_t * val__sdatetime;
static ti_val_t * val__sint;
static ti_val_t * val__sfloat;
static ti_val_t * val__sstr;
static ti_val_t * val__sbytes;
static ti_val_t * val__sinfo;
static ti_val_t * val__sregex;
static ti_val_t * val__serror;
static ti_val_t * val__sclosure;
static ti_val_t * val__slist;
static ti_val_t * val__stuple;
static ti_val_t * val__sset;
static ti_val_t * val__sthing;
static ti_val_t * val__swthing;
static ti_val_t * val__tar_gz_str;
static ti_val_t * val__gs_str;
static ti_val_t * val__charset_str;
static ti_val_t * val__year_name;
static ti_val_t * val__month_name;
static ti_val_t * val__day_name;
static ti_val_t * val__hour_name;
static ti_val_t * val__minute_name;
static ti_val_t * val__second_name;
static ti_val_t * val__gmt_offset_name;


#define VAL__BUF_SZ 128
static char val__buf[VAL__BUF_SZ];

static ti_val_t * val__unp_map(ti_vup_t * vup, size_t sz, ex_t * e)
{
    mp_obj_t mp_key, mp_val;
    const char * restore_point;
    if (!sz)
        return (ti_val_t *) ti_thing_new_from_vup(vup, sz, e);

    restore_point = vup->up->pt;

    if (mp_next(vup->up, &mp_key) != MP_STR || mp_key.via.str.n == 0)
    {
        ex_set(e, EX_TYPE_ERROR,
                "property names must be of type `"TI_VAL_STR_S"` "
                "and follow the naming rules"DOC_NAMES);
        return NULL;
    }

    switch ((ti_val_kind) *mp_key.via.str.data)
    {
    case TI_KIND_C_THING:
        if (!vup->collection)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot unpack a `thing` without a collection");
            return NULL;
        }
        if (mp_next(vup->up, &mp_val) <= 0 || mp_cast_u64(&mp_val))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting an integer value as thing id");
            return NULL;
        }
        return (ti_val_t *) ti_things_thing_o_from_vup(
                vup,
                mp_val.via.u64,
                sz,
                e);
    case TI_KIND_C_INSTANCE:
        if (!vup->collection)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot unpack a `thing` without a collection");
            return NULL;
        }
        if (sz != 3)
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting exactly 3 key value pairs for a type");
            return NULL;
        }
        return (ti_val_t *) ti_things_thing_t_from_vup(vup, e);
    case TI_KIND_C_CLOSURE:
    {
        ti_qbind_t syntax = {
                .val_cache_n = 0,
                .flags = vup->collection
                    ? TI_QBIND_FLAG_COLLECTION
                    : TI_QBIND_FLAG_THINGSDB,
        };
        if (sz != 1 || mp_next(vup->up, &mp_val) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "closures must be written according the following syntax: "
                    "{\""TI_KIND_S_CLOSURE"\": \"...\"");
            return NULL;
        }

        return (ti_val_t *) ti_closure_from_strn(
                &syntax,
                mp_val.via.str.data,
                mp_val.via.str.n, e);
    }
    case TI_KIND_C_REGEX:
    {
        if (sz != 1 || mp_next(vup->up, &mp_val) != MP_STR)        {
            ex_set(e, EX_BAD_DATA,
                    "regular expressions must be written according the "
                    "following syntax: {\""TI_KIND_S_REGEX"\": \"...\"");
            return NULL;
        }

        return (ti_val_t *) ti_regex_from_strn(
                mp_val.via.str.data,
                mp_val.via.str.n, e);
    }
    case TI_KIND_C_SET:
    {
        ti_val_t * vthing;
        ti_vset_t * vset = ti_vset_create();
        size_t i, n;
        if (!vset)
        {
            ex_set_mem(e);
            return NULL;
        }
        if (sz != 1 || mp_next(vup->up, &mp_val) != MP_ARR)
        {
            ex_set(e, EX_BAD_DATA,
                    "sets must be written according the "
                    "following syntax: {\""TI_KIND_S_SET"\": [...]");
            return NULL;
        }

        for (i = 0, n = mp_val.via.sz; i < n; ++i)
        {
            if (mp_next(vup->up, &mp_val) != MP_MAP)
            {
                ti_vset_destroy(vset);
                ex_set(e, EX_TYPE_ERROR, "sets can only contain things");
                return NULL;
            }

            vthing = val__unp_map(vup, mp_val.via.sz, e);
            if (!vthing || (ti_vset_add_val(vset, vthing, e) < 0))
            {
                ti_vset_destroy(vset);
                return NULL;  /* `e` is set in both cases */
            }
        }

        return (ti_val_t *) vset;
    }
    case TI_KIND_C_ERROR:
    {
        ti_verror_t * verror;
        mp_obj_t mp_msg, mp_code;
        if (sz != 3 ||
            mp_skip(vup->up) != MP_STR ||       /* first value: definition */
            mp_skip(vup->up) != MP_STR ||       /* key: error_msg */
            mp_next(vup->up, &mp_msg) != MP_STR ||
            mp_msg.via.str.n > EX_MAX_SZ ||
            mp_skip(vup->up) != MP_STR ||       /* error_code */
            mp_next(vup->up, &mp_code) != MP_I64 ||
            mp_code.via.i64 < EX_MIN_ERR ||
            mp_code.via.i64 > EX_MAX_BUILD_IN_ERR)
        {
            ex_set(e, EX_BAD_DATA,
                    "errors must be written according the "
                    "following syntax: {"
                    "\""TI_KIND_S_ERROR"\": \"..err()\","
                    "\"error_msg\": \"..msg\","
                    "\"error_code\": code}");
            return NULL;
        }

        verror = ti_verror_create(
                mp_msg.via.str.data,
                mp_msg.via.str.n,
                mp_code.via.i64);

        if (!verror)
            ex_set_mem(e);

        return (ti_val_t *) verror;
    }
    case TI_KIND_C_WRAP:
    {
        mp_obj_t mp_type_id;
        ti_val_t * vthing;
        ti_wrap_t * wrap;

        if (sz != 1 ||
            mp_next(vup->up, &mp_val) != MP_ARR || mp_val.via.sz != 2 ||
            mp_next(vup->up, &mp_type_id) != MP_U64 ||
            mp_next(vup->up, &mp_val) != MP_MAP || mp_val.via.sz < 1)
        {
            ex_set(e, EX_BAD_DATA,
                "wrap type must be written according the "
                "following syntax: {\""TI_KIND_S_WRAP"\": [type_id, {...}]");
            return NULL;
        }

        vthing = val__unp_map(vup, mp_val.via.sz, e);
        if (!vthing)
            return NULL;

        if (!ti_val_is_thing(vthing))
        {
            ex_set(e, EX_BAD_DATA,
                "wrap type is expecting a wrapped `"TI_VAL_THING_S"` but "
                "got type `%s` instead",
                ti_val_str(vthing));
            ti_val_unsafe_drop(vthing);
            return NULL;
        }

        wrap = ti_wrap_create((ti_thing_t *) vthing, mp_type_id.via.u64);
        if (!wrap)
            ex_set_mem(e);
        return (ti_val_t *) wrap;
    }
    case TI_KIND_C_MEMBER:
    {
        mp_obj_t mp_enum_id, mp_idx;
        ti_enum_t * enum_;
        ti_member_t * member;

        if (sz != 1 ||
            mp_next(vup->up, &mp_val) != MP_ARR || mp_val.via.sz != 2 ||
            mp_next(vup->up, &mp_enum_id) != MP_U64 ||
            mp_next(vup->up, &mp_idx) != MP_U64)
        {
            ex_set(e, EX_BAD_DATA,
                "enum type must be written according the "
                "following syntax: {\""TI_KIND_S_MEMBER"\": [enum_id, index]");
            return NULL;
        }

        enum_= ti_enums_by_id(vup->collection->enums, mp_enum_id.via.u64);
        if (!enum_)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "cannot find enum with id %"PRIu64,
                    mp_enum_id.via.u64);
            return NULL;
        }

        member = ti_enum_member_by_idx(enum_, mp_idx.via.u64);
        if (!member)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "internal index out of range in enumerator `%s`",
                    enum_->name);
            return NULL;
        }
        ti_incref(member);
        return (ti_val_t *) member;
    }
    case TI_KIND_C_DATETIME:
    {
        mp_obj_t mp_ts, mp_offset, mp_tz;
        ti_datetime_t * dt;
        ti_tz_t * tz;

        if (sz != 1 ||
            mp_next(vup->up, &mp_val) != MP_ARR || mp_val.via.sz != 3 ||
            mp_next(vup->up, &mp_ts) <= 0 || mp_cast_i64(&mp_ts) ||
            mp_next(vup->up, &mp_offset) <= 0 || mp_cast_i64(&mp_ts) ||
            mp_next(vup->up, &mp_tz) <= 0)  /* U64 or NIL */
        {
            ex_set(e, EX_BAD_DATA,
                "datetime type must be written according the "
                "following syntax: {\""TI_KIND_S_DATETIME"\": "
                "[ts, offset, tz_info]");
            return NULL;
        }

        if (mp_offset.via.i64 < DATETIME_OFFSET_MIN ||
            mp_offset.via.i64 > DATETIME_OFFSET_MAX)
        {
            ex_set(e, EX_BAD_DATA,
                "invalid datetime offset: %"PRId64, mp_offset.via.i64);
            return NULL;
        }

        if (mp_tz.tp == MP_U64)
        {
            tz = ti_tz_from_index(mp_tz.via.u64);
        }
        else if (mp_tz.tp == MP_NIL)
        {
            tz = NULL;
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                    "invalid timezone, expecting `nil` or `u64`");
            return NULL;
        }

        dt = ti_datetime_from_i64(mp_ts.via.i64, mp_offset.via.i64, tz);
        if (!dt)
            ex_set_mem(e);

        return (ti_val_t *) dt;
    }
    }

    /* restore the unpack pointer to the first property */
    vup->up->pt = restore_point;
    return (ti_val_t *) ti_thing_new_from_vup(vup, sz, e);
}

/*
 * Does not increment the `val` reference counter.
 */
static int val__push(ti_varr_t * varr, ti_val_t * val, ex_t * e)
{
    assert (ti_varr_is_list(varr));

    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:
    case TI_VAL_WRAP:
        varr->flags |= TI_VFLAG_ARR_MHT;
        break;
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        break;
    case TI_VAL_ARR:
    {
        /* Make sure the arr is converted to a `tuple` and copy the
         * may-have-things flags to the parent.
         */
        ti_varr_t * arr = (ti_varr_t *) val;
        arr->flags |= TI_VFLAG_ARR_TUPLE;
        varr->flags |= arr->flags & TI_VFLAG_ARR_MHT;
        break;
    }
    case TI_VAL_SET:
        /* This should never happen since a `set` could never be part of the
         * array in the first place.
         */
        ex_set(e, EX_TYPE_ERROR,
                "unexpected `set` which cannot be added to the array");
        return e->nr;
    case TI_VAL_MEMBER:
        if (ti_val_is_thing(VMEMBER(val)))
            varr->flags |= TI_VFLAG_ARR_MHT;
        break;
    case TI_VAL_TEMPLATE:
        assert (0);
        return e->nr;
    }

    if (vec_push(&varr->vec, val))
        ex_set_mem(e);
    return e->nr;
}

/*
 * Return NULL when failed and `e` is set to an appropriate error message
 */
ti_val_t * ti_val_from_vup_e(ti_vup_t * vup, ex_t * e)
{
    mp_obj_t obj;
    mp_enum_t tp = mp_next(vup->up, &obj);
    switch(tp)
    {
    case MP_NEVER_USED:
    case MP_INCOMPLETE:
    case MP_ERR:
        ex_set(e, EX_BAD_DATA, mp_type_str(tp));
        return NULL;
    case MP_END:
        ex_set(e, EX_NUM_ARGUMENTS, "missing value");
        return NULL;
    case MP_I64:
    {
        ti_vint_t * vint = ti_vint_create(obj.via.i64);
        if (!vint)
            ex_set_mem(e);
        return (ti_val_t *) vint;
    }
    case MP_U64:
    {
        if (obj.via.u64 > INT64_MAX)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return NULL;
        }
        ti_vint_t * vint = ti_vint_create((int64_t) obj.via.u64);
        if (!vint)
            ex_set_mem(e);
        return (ti_val_t *) vint;
    }
    case MP_F64:
    {
        ti_vfloat_t * vfloat = ti_vfloat_create(obj.via.f64);
        if (!vfloat)
            ex_set_mem(e);
        return (ti_val_t *) vfloat;
    }
    case MP_BIN:
    {
        ti_raw_t * raw = ti_bin_create(obj.via.bin.data, obj.via.bin.n);
        if (!raw)
            ex_set_mem(e);
        return (ti_val_t *) raw;
    }
    case MP_STR:
    {
        ti_raw_t * raw = ti_str_create(obj.via.str.data, obj.via.str.n);
        if (!raw)
            ex_set_mem(e);
        return (ti_val_t *) raw;
    }
    case MP_BOOL:
        return (ti_val_t *) ti_vbool_get(obj.via.bool_);
    case MP_NIL:
        return (ti_val_t *) ti_nil_get();
    case MP_ARR:
    {
        size_t n = obj.via.sz;
        ti_val_t * v;
        ti_varr_t * varr = ti_varr_create(n);
        if (!varr)
        {
            ex_set_mem(e);
            return NULL;
        }
        while (n--)
        {
            v = ti_val_from_vup_e(vup, e);
            if (!v || val__push(varr, v, e))
            {
                ti_val_drop(v);
                ti_val_unsafe_drop((ti_val_t *) varr);
                return NULL;  /* error `e` is set in both cases */
            }
        }
        return (ti_val_t *) varr;
    }
    case MP_MAP:
        return val__unp_map(vup, obj.via.sz, e);
    case MP_EXT:
    {
        ti_raw_t * raw;
        if (obj.via.ext.tp != TI_STR_INFO)
        {
            ex_set(e, EX_BAD_DATA,
                    "msgpack extension type %d is not supported by ThingsDB",
                    obj.via.ext.tp);
        }
        raw = ti_mp_create(obj.via.ext.data, obj.via.ext.n);
        if (!raw)
            ex_set_mem(e);
        return (ti_val_t *) raw;
    }
    }

    ex_set(e, EX_BAD_DATA, "unexpected code reached while unpacking value");
    return NULL;
}

int ti_val_init_common(void)
{
    val__empty_bin = (ti_val_t *) ti_bin_create(NULL, 0);
    val__empty_str = (ti_val_t *) ti_str_from_str("");
    val__sany = (ti_val_t *) ti_str_from_str("any");
    val__snil = (ti_val_t *) ti_str_from_str(TI_VAL_NIL_S);
    val__strue = (ti_val_t *) ti_str_from_str("true");
    val__sfalse = (ti_val_t *) ti_str_from_str("false");
    val__sbool = (ti_val_t *) ti_str_from_str(TI_VAL_BOOL_S);
    val__sdatetime = (ti_val_t *) ti_str_from_str(TI_VAL_DATETIME_S);
    val__sint = (ti_val_t *) ti_str_from_str(TI_VAL_INT_S);
    val__sfloat = (ti_val_t *) ti_str_from_str(TI_VAL_FLOAT_S);
    val__sstr = (ti_val_t *) ti_str_from_str(TI_VAL_STR_S);
    val__sbytes = (ti_val_t *) ti_str_from_str(TI_VAL_BYTES_S);
    val__sinfo = (ti_val_t *) ti_str_from_str(TI_VAL_INFO_S);
    val__sregex = (ti_val_t *) ti_str_from_str(TI_VAL_REGEX_S);
    val__serror = (ti_val_t *) ti_str_from_str(TI_VAL_ERROR_S);
    val__sclosure = (ti_val_t *) ti_str_from_str(TI_VAL_CLOSURE_S);
    val__slist = (ti_val_t *) ti_str_from_str(TI_VAL_LIST_S);
    val__stuple = (ti_val_t *) ti_str_from_str(TI_VAL_TUPLE_S);
    val__sset = (ti_val_t *) ti_str_from_str(TI_VAL_SET_S);
    val__sthing = (ti_val_t *) ti_str_from_str(TI_VAL_THING_S);
    val__swthing = (ti_val_t *) ti_str_from_str("<thing>");
    val__tar_gz_str = (ti_val_t *) ti_str_from_str(".tar.gz");
    val__gs_str = (ti_val_t *) ti_str_from_str("gs://");
    val__charset_str = (ti_val_t *) ti_str_from_str(
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "-_");

    /* names */
    val__year_name = (ti_val_t *) ti_names_from_str("year");
    val__month_name = (ti_val_t *) ti_names_from_str("month");
    val__day_name = (ti_val_t *) ti_names_from_str("day");
    val__hour_name = (ti_val_t *) ti_names_from_str("hour");
    val__minute_name = (ti_val_t *) ti_names_from_str("minute");
    val__second_name = (ti_val_t *) ti_names_from_str("second");
    val__gmt_offset_name = (ti_val_t *) ti_names_from_str("gmt_offset");


    if (!val__empty_bin || !val__empty_str || !val__snil || !val__strue ||
        !val__sfalse || !val__sbool || !val__sdatetime || !val__sint ||
        !val__sfloat || !val__sstr || !val__sbytes || !val__sinfo ||
        !val__sregex || !val__serror || !val__sclosure || !val__slist ||
        !val__stuple || !val__sset || !val__sthing || !val__swthing ||
        !val__tar_gz_str || !val__sany || !val__gs_str || !val__charset_str ||
        !val__year_name || !val__month_name || !val__day_name ||
        !val__hour_name || !val__minute_name || !val__second_name ||
        !val__gmt_offset_name)
    {
        ti_val_drop_common();
        return -1;
    }
    return 0;
}

void ti_val_drop_common(void)
{
    ti_val_drop(val__empty_bin);
    ti_val_drop(val__empty_str);
    ti_val_drop(val__sany);
    ti_val_drop(val__snil);
    ti_val_drop(val__strue);
    ti_val_drop(val__sfalse);
    ti_val_drop(val__sbool);
    ti_val_drop(val__sdatetime);
    ti_val_drop(val__sint);
    ti_val_drop(val__sfloat);
    ti_val_drop(val__sstr);
    ti_val_drop(val__sbytes);
    ti_val_drop(val__sinfo);
    ti_val_drop(val__sregex);
    ti_val_drop(val__serror);
    ti_val_drop(val__sclosure);
    ti_val_drop(val__slist);
    ti_val_drop(val__stuple);
    ti_val_drop(val__sset);
    ti_val_drop(val__sthing);
    ti_val_drop(val__swthing);
    ti_val_drop(val__tar_gz_str);
    ti_val_drop(val__gs_str);
    ti_val_drop(val__charset_str);
}

void ti_val_destroy(ti_val_t * val)
{
    assert (val);
    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_BOOL:
        assert (0);     /* there should always be one reference
                           left for nil and boolean. */
        return;
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_MP:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_ERROR:
    case TI_VAL_DATETIME:
        break;
    case TI_VAL_NAME:
        ti_name_destroy((ti_name_t *) val);
        return;
    case TI_VAL_REGEX:
        ti_regex_destroy((ti_regex_t *) val);
        return;
    case TI_VAL_THING:
        ti_thing_destroy((ti_thing_t *) val);
        return;
    case TI_VAL_WRAP:
        ti_wrap_destroy((ti_wrap_t *) val);
        return;
    case TI_VAL_ARR:
        ti_varr_destroy((ti_varr_t *) val);
        return;
    case TI_VAL_SET:
        ti_vset_destroy((ti_vset_t *) val);
        return;
    case TI_VAL_CLOSURE:
        ti_closure_destroy((ti_closure_t *) val);
        return;
    case TI_VAL_MEMBER:
        ti_member_destroy((ti_member_t *) val);
        return;
    case TI_VAL_TEMPLATE:
        ti_template_destroy((ti_template_t *) val);
        return;
    }

    free(val);
}

int ti_val_make_int(ti_val_t ** val, int64_t i)
{
    ti_val_t * v = (ti_val_t *) ti_vint_create(i);
    if (!v)
        return -1;

    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}

int ti_val_make_float(ti_val_t ** val, double d)
{
    ti_val_t * v = (ti_val_t *) ti_vfloat_create(d);
    if (!v)
        return -1;

    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}

/*
 * Return NULL when failed. Otherwise a new value with a reference.
 */
ti_val_t * ti_val_from_vup(ti_vup_t * vup)
{
    ex_t e = {0};
    ti_val_t * val;

    val = ti_val_from_vup_e(vup, &e);
    if (e.nr)
        log_error("failed to unpack value: %s (%d) ", e.msg, e.nr);
    return val;
}

ti_val_t * ti_val_empty_str(void)
{
    ti_incref(val__empty_str);
    return val__empty_str;
}

ti_val_t * ti_val_charset_str(void)
{
    ti_incref(val__charset_str);
    return val__charset_str;
}

ti_val_t * ti_val_borrow_any_str(void)
{
    return val__sany;
}

ti_val_t * ti_val_borrow_tar_gz_str(void)
{
    return val__tar_gz_str;
}

ti_val_t * ti_val_borrow_gs_str(void)
{
    return val__gs_str;
}

ti_val_t * ti_val_empty_bin(void)
{
    ti_incref(val__empty_bin);
    return val__empty_bin;
}

ti_val_t * ti_val_wthing_str(void)
{
    ti_incref(val__swthing);
    return val__swthing;
}

ti_val_t * ti_val_year_name(void)
{
    ti_incref(val__year_name);
    return val__year_name;
}

ti_val_t * ti_val_month_name(void)
{
    ti_incref(val__month_name);
    return val__month_name;
}

ti_val_t * ti_val_day_name(void)
{
    ti_incref(val__day_name);
    return val__day_name;
}

ti_val_t * ti_val_hour_name(void)
{
    ti_incref(val__hour_name);
    return val__hour_name;
}

ti_val_t * ti_val_minute_name(void)
{
    ti_incref(val__minute_name);
    return val__minute_name;
}

ti_val_t * ti_val_second_name(void)
{
    ti_incref(val__second_name);
    return val__second_name;
}

ti_val_t * ti_val_gmt_offset_name(void)
{
    ti_incref(val__gmt_offset_name);
    return val__gmt_offset_name;
}



/*
 * Returns an address to the `access` object by a given value and set the
 * argument `scope_id` to the collection ID, or TI_SCOPE_THINGSDB or
 * TI_SCOPE_NODE. If no valid scope is found, `e` will be set with an
 * appropriate error message.
 */
vec_t ** ti_val_get_access(ti_val_t * val, ex_t * e, uint64_t * scope_id)
{
    ti_collection_t * collection;
    ti_raw_t * raw = (ti_raw_t *) val;
    ti_scope_t scope;

    if (!ti_val_is_str(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting a scope to be of type `"TI_VAL_STR_S"` "
                "but got type `%s` instead",
                ti_val_str(val));
        return NULL;
    }

    if (ti_scope_init(&scope, (const char *) raw->data, raw->n, e))
        return NULL;

    switch (scope.tp)
    {
    case TI_SCOPE_THINGSDB:
        *scope_id = TI_SCOPE_THINGSDB;
        return &ti.access_thingsdb;
    case TI_SCOPE_NODE:
        *scope_id = TI_SCOPE_NODE;
        return &ti.access_node;
    case TI_SCOPE_COLLECTION_NAME:
        collection = ti_collections_get_by_strn(
                scope.via.collection_name.name,
                scope.via.collection_name.sz);
        if (collection)
        {
            *scope_id = collection->root->id;
            return &collection->access;
        }

        ex_set(e, EX_LOOKUP_ERROR, "collection `%.*s` not found",
                (int) scope.via.collection_name.sz,
                scope.via.collection_name.name);
        return NULL;
    case TI_SCOPE_COLLECTION_ID:
        collection = ti_collections_get_by_id(scope.via.collection_id);
        if (collection)
        {
            *scope_id = collection->root->id;
            return &collection->access;
        }

        ex_set(e, EX_LOOKUP_ERROR, TI_COLLECTION_ID" not found",
                scope.via.collection_id);
        return NULL;
    }

    assert (0);
    return NULL;
}

int ti_val_convert_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = NULL;

    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
        v = val__snil;
        ti_incref(v);
        break;

    case TI_VAL_INT:
    {
        size_t n;
        const char * s = strx_from_int64(VINT(*val), &n);
        v = (ti_val_t *) ti_str_create(s, n);
        if (!v)
        {
            ex_set_mem(e);
            return e->nr;
        }
        break;
    }
    case TI_VAL_FLOAT:
    {
        size_t n;
        const char * s = strx_from_double(VFLOAT(*val), &n);
        v = (ti_val_t *) ti_str_create(s, n);
        if (!v)
        {
            ex_set_mem(e);
            return e->nr;
        }
        break;
    }
    case TI_VAL_BOOL:
        v = VBOOL(*val) ? val__strue : val__sfalse;
        ti_incref(v);
        break;

    case TI_VAL_DATETIME:
        v = (ti_val_t *) ti_datetime_to_str((ti_datetime_t *) *val, e);
        if (!v)
            return e->nr;
        break;

    case TI_VAL_NAME:
    case TI_VAL_STR:
        return e->nr;  /* do nothing, just return the string */
    case TI_VAL_BYTES:
    {
        ti_raw_t * r = (ti_raw_t *) (*val);
        if (!strx_is_utf8n((const char *) r->data, r->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "binary data has no valid UTF8 encoding");
            return e->nr;
        }
        if (r->ref == 1)
        {
            /* only one reference left we can just change the type */
            r->tp = TI_VAL_STR;
            return e->nr;
        }
        v = (ti_val_t *) ti_str_create((const char *) r->data, r->n);
        if (!v)
        {
            ex_set_mem(e);
            return e->nr;
        }
        break;
    }
    case TI_VAL_REGEX:
        v = (ti_val_t *) (*(ti_regex_t **) val)->pattern;
        ti_incref(v);
        break;
    case TI_VAL_MP:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_STR_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_ERROR:
        v = (ti_val_t *) ti_str_create(
                (*(ti_verror_t **) val)->msg,
                (*(ti_verror_t **) val)->msg_n);
        if (!v)
            return -1;
        break;
    case TI_VAL_MEMBER:
        v = VMEMBER(val);
        ti_incref(v);
        if (ti_val_convert_to_str(&v, e))
        {
            ti_decref(v);
            return e->nr;
        }
        break;
    case TI_VAL_TEMPLATE:
        assert(0);
    }

    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}

int ti_val_convert_to_bytes(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = NULL;

    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NAME:
    case TI_VAL_STR:
    {
        ti_raw_t * r = (ti_raw_t *) (*val);
        if (r->ref == 1 && r->tp == TI_VAL_STR)
        {
            /* only one reference left we can just change the type */
            r->tp = TI_VAL_BYTES;
            return e->nr;
        }
        v = (ti_val_t *) ti_bin_create(r->data, r->n);
        if (!v)
        {
            ex_set_mem(e);
            return e->nr;
        }
        break;
    }
    case TI_VAL_BYTES:
        return e->nr;  /* do nothing, just return the string */
    case TI_VAL_REGEX:
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MP:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_BYTES_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_MEMBER:
        v = VMEMBER(val);
        ti_incref(v);
        if (ti_val_convert_to_bytes(&v, e))
        {
            ti_decref(v);
            return e->nr;
        }
        break;
    case TI_VAL_TEMPLATE:
        assert(0);
    }

    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}

int ti_val_convert_to_int(ti_val_t ** val, ex_t * e)
{
    int64_t i = 0;
    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_MP:
    case TI_VAL_BYTES:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_INT_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_INT:
        return 0;
    case TI_VAL_FLOAT:
        if (ti_val_overflow_cast(VFLOAT(*val)))
            goto overflow;

        i = (int64_t) VFLOAT(*val);
        break;
    case TI_VAL_BOOL:
        i = VBOOL(*val);
        break;
    case TI_VAL_DATETIME:
        i = DATETIME(*val);
        break;
    case TI_VAL_NAME:
    case TI_VAL_STR:
    {
        ti_raw_t * raw = *((ti_raw_t **) val);

        if (errno == ERANGE)
            errno = 0;

        if (raw->n >= VAL__BUF_SZ)
        {
            char * dup = strndup((const char *) raw->data, raw->n);
            if (!dup)
            {
                ex_set_mem(e);
                return e->nr;
            }
            i = strtoll(dup, NULL, 0);
            free(dup);
        }
        else
        {
            memcpy(val__buf, raw->data, raw->n);
            val__buf[raw->n] = '\0';
            i = strtoll(val__buf, NULL, 0);
        }
        if (errno == ERANGE)
            goto overflow;

        break;
    }
    case TI_VAL_ERROR:
        i = (*((ti_verror_t **) val))->code;
        break;
    case TI_VAL_MEMBER:
    {
        ti_val_t * v = VMEMBER(*val);
        ti_incref(v);
        if (ti_val_convert_to_int(&v, e))
        {
            ti_decref(v);
            return e->nr;
        }
        ti_val_unsafe_drop(*val);
        *val = v;
        return 0;
    }
    case TI_VAL_TEMPLATE:
        assert(0);
    }

    if (ti_val_make_int(val, i))
        ex_set_mem(e);

    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}

int ti_val_convert_to_float(ti_val_t ** val, ex_t * e)
{
    double d = 0.0;
    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_MP:
    case TI_VAL_BYTES:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_FLOAT_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_INT:
        d = (double) VINT(*val);
        break;
    case TI_VAL_FLOAT:
        return 0;
    case TI_VAL_BOOL:
        d = (double) VBOOL(*val);
        break;
    case TI_VAL_DATETIME:
        d = (double) DATETIME(*val);
        break;
    case TI_VAL_NAME:
    case TI_VAL_STR:
    {
        ti_raw_t * raw = *((ti_raw_t **) val);

        if (errno == ERANGE)
            errno = 0;

        if (raw->n >= VAL__BUF_SZ)
        {
            char * dup = strndup((const char *) raw->data, raw->n);
            if (!dup)
            {
                ex_set_mem(e);
                return e->nr;
            }
            d = strtod(dup, NULL);
            free(dup);
        }
        else
        {
            memcpy(val__buf, raw->data, raw->n);
            val__buf[raw->n] = '\0';
            d = strtod(val__buf, NULL);
        }
        if (errno == ERANGE)
        {
            assert (d == HUGE_VAL || d == -HUGE_VAL);
            d = d == HUGE_VAL ? INFINITY : -INFINITY;
            errno = 0;
        }

        break;
    }
    case TI_VAL_ERROR:
        d = (double) (*(ti_verror_t **) val)->code;
        break;
    case TI_VAL_MEMBER:
    {
        ti_val_t * v = VMEMBER(*val);
        ti_incref(v);
        if (ti_val_convert_to_float(&v, e))
        {
            ti_decref(v);
            return e->nr;
        }
        ti_val_unsafe_drop(*val);
        *val = v;
        return 0;
    }
    case TI_VAL_TEMPLATE:
        assert(0);
    }

    if (ti_val_make_float(val, d))
        ex_set_mem(e);

    return e->nr;
}

int ti_val_convert_to_array(ti_val_t ** val, ex_t * e)
{
    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_LIST_S"`",
                ti_val_str(*val));
        break;
    case TI_VAL_ARR:
        break;
    case TI_VAL_SET:
        if (ti_vset_to_list((ti_vset_t **) val))
            ex_set_mem(e);
        break;
    case TI_VAL_TEMPLATE:
        assert(0);
    }
    return e->nr;
}

int ti_val_convert_to_set(ti_val_t ** val, ex_t * e)
{
    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_WRAP:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_SET_S"`",
                ti_val_str(*val));
        break;
    case TI_VAL_THING:
    {
        ti_vset_t * vset = ti_vset_create();
        if (!vset || ti_vset_add(vset, (ti_thing_t *) *val))
        {
            ti_vset_destroy(vset);
            ex_set_mem(e);
            break;
        }

        *val = (ti_val_t *) vset;
        break;
    }
    case TI_VAL_ARR:
    {
        vec_t * vec = VARR(*val);
        ti_vset_t * vset = ti_vset_create();
        if (!vset)
        {
            ex_set_mem(e);
            break;
        }

        for (vec_each(vec, ti_val_t, v))
        {
            if (ti_vset_add_val(vset, v, e) < 0)
            {
                ti_vset_destroy(vset);
                return e->nr;
            }
        }

        ti_val_unsafe_drop(*val);
        *val = (ti_val_t *) vset;
        break;
    }
    case TI_VAL_SET:
        break;
    case TI_VAL_TEMPLATE:
        assert(0);
    }
    return e->nr;
}

/*
 * Can be called on any value type and returns `true` or `false`.
 */
_Bool ti_val_as_bool(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        return false;
    case TI_VAL_INT:
        return !!VINT(val);
    case TI_VAL_FLOAT:
        return !!VFLOAT(val);
    case TI_VAL_BOOL:
        return VBOOL(val);
    case TI_VAL_DATETIME:
        return true;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        return !!((ti_raw_t *) val)->n;
    case TI_VAL_REGEX:
        return true;
    case TI_VAL_ARR:
        return !!VARR(val)->n;
    case TI_VAL_SET:
        return !!VSET(val)->n;
    case TI_VAL_THING:
        return !!((ti_thing_t *) val)->items->n;
    case TI_VAL_WRAP:
        return !!((ti_wrap_t *) val)->thing->items->n;
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        return true;
    case TI_VAL_MEMBER:
        return ti_val_as_bool(VMEMBER(val));
    case TI_VAL_TEMPLATE:
        assert(0);
    }
    assert (0);
    return false;
}

/*
 * Can only be called on values which have a length.
 * Enum types should have been checked on the value they contain.
 */
size_t ti_val_get_len(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MP:
    case TI_VAL_TEMPLATE:
        break;
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        return ((ti_raw_t *) val)->n;
    case TI_VAL_REGEX:
        break;
    case TI_VAL_THING:
        return ((ti_thing_t *) val)->items->n;
    case TI_VAL_WRAP:
        return ((ti_wrap_t *) val)->thing->items->n;
    case TI_VAL_ARR:
        return VARR(val)->n;
    case TI_VAL_SET:
        return VSET(val)->n;
    case TI_VAL_CLOSURE:
        break;
    case TI_VAL_MEMBER:
        return ti_val_get_len(VMEMBER(val));
    case TI_VAL_ERROR:
        break;
        assert(0);
    }
    return 0;
}

static inline int val__walk_set(ti_thing_t * thing, void * UNUSED(_))
{
    return !thing->id && ti_thing_gen_id(thing);
}

/*
 * Must be called when, and only when generating a new `task`. New ID's must
 * also be present in a task so other nodes will consume the same ID's.
 */
int ti_val_gen_ids(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_MEMBER:
        /* enum can be skipped; even a thing on an enum is guaranteed to
         * have an ID since they are triggered when creating or changing the
         * enumerator
         */
        break;

    case TI_VAL_THING:
        if (!((ti_thing_t *) val)->id)
            return ti_thing_gen_id((ti_thing_t *) val);
        /*
         * New things 'under' an existing thing will get their own event,
         * so here we do not need recursion.
         */
        break;
    case TI_VAL_WRAP:
        if (!((ti_wrap_t *) val)->thing->id)
            return ti_thing_gen_id(((ti_wrap_t *) val)->thing);
        /*
         * New things 'under' an existing thing will get their own event,
         * so here we do not need recursion.
         */
        break;
    case TI_VAL_ARR:
        /*
         * Here the code really benefits from the `may-have-things` flag since
         * must attached arrays will contain "only" things, or no things.
         */
        if (ti_varr_may_have_things((ti_varr_t *) val))
            for (vec_each(VARR(val), ti_val_t, v))
                if (ti_val_gen_ids(v))
                    return -1;
        break;
    case TI_VAL_SET:
        return imap_walk(VSET(val), (imap_cb) val__walk_set, NULL);
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        break;
    case TI_VAL_TEMPLATE:
        assert (0);
    }
    return 0;
}

int ti_val_to_pk(ti_val_t * val, msgpack_packer * pk, int options)
{
    switch ((ti_val_enum) val->tp)
    {
    TI_VAL_PACK_CASE_IMMUTABLE(val, pk, options)
    case TI_VAL_THING:
        return ti_thing_to_pk((ti_thing_t *) val, pk, options);
    case TI_VAL_WRAP:
        return ti_wrap_to_pk((ti_wrap_t *) val, pk, options);
    case TI_VAL_ARR:
    {
        ti_varr_t * varr = (ti_varr_t *) val;
        if (msgpack_pack_array(pk, varr->vec->n))
            return -1;
        for (vec_each(varr->vec, ti_val_t, v))
            if (ti_val_to_pk(v, pk, options))
                return -1;
        return 0;
    }
    case TI_VAL_SET:
        return ti_vset_to_pk((ti_vset_t *) val, pk, options);
    case TI_VAL_MEMBER:
        return ti_member_to_pk((ti_member_t *) val, pk, options);
    }

    assert(0);
    return -1;
}

void ti_val_may_change_pack_sz(ti_val_t * val, size_t * sz)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        *sz = 32;
        return;
    case TI_VAL_DATETIME:
        *sz = 64;
        return;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        *sz = ((ti_raw_t *) val)->n + 32;
        return;
    case TI_VAL_REGEX:
        *sz = ((ti_regex_t *) val)->pattern->n + 32;
        return;
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
        *sz = 65536;
        return;
    case TI_VAL_CLOSURE:
        *sz = 8192;
        return;
    case TI_VAL_ERROR:
        *sz = ((ti_verror_t *) val)->msg_n + 96;
        return;
    case TI_VAL_MEMBER:
        ti_val_may_change_pack_sz(VMEMBER(val), sz);
        return;
    case TI_VAL_TEMPLATE:
        assert (0);
    }
}

const char * ti_val_str(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:            return TI_VAL_NIL_S;
    case TI_VAL_INT:            return TI_VAL_INT_S;
    case TI_VAL_FLOAT:          return TI_VAL_FLOAT_S;
    case TI_VAL_BOOL:           return TI_VAL_BOOL_S;
    case TI_VAL_DATETIME:       return TI_VAL_DATETIME_S;
    case TI_VAL_MP:             return TI_VAL_INFO_S;
    case TI_VAL_NAME:
    case TI_VAL_STR:            return TI_VAL_STR_S;
    case TI_VAL_BYTES:          return TI_VAL_BYTES_S;
    case TI_VAL_REGEX:          return TI_VAL_REGEX_S;
    case TI_VAL_THING:          return ti_thing_is_object((ti_thing_t *) val)
                                    ? TI_VAL_THING_S
                                    : ti_thing_type_str((ti_thing_t *) val);
    case TI_VAL_WRAP:           return ti_wrap_str((ti_wrap_t *) val);
    case TI_VAL_ARR:            return ti_varr_is_list((ti_varr_t *) val)
                                    ? TI_VAL_LIST_S
                                    : TI_VAL_TUPLE_S;
    case TI_VAL_SET:            return TI_VAL_SET_S;
    case TI_VAL_CLOSURE:        return TI_VAL_CLOSURE_S;
    case TI_VAL_ERROR:          return TI_VAL_ERROR_S;
    case TI_VAL_MEMBER:
        return ti_member_enum_name((ti_member_t *) val);
    case TI_VAL_TEMPLATE:
        assert (0);
    }
    assert (0);
    return "unknown";
}

ti_val_t * ti_val_strv(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:            return ti_grab(val__snil);
    case TI_VAL_INT:            return ti_grab(val__sint);
    case TI_VAL_FLOAT:          return ti_grab(val__sfloat);
    case TI_VAL_BOOL:           return ti_grab(val__sbool);
    case TI_VAL_DATETIME:       return ti_grab(val__sdatetime);
    case TI_VAL_MP:             return ti_grab(val__sinfo);
    case TI_VAL_NAME:
    case TI_VAL_STR:            return ti_grab(val__sstr);
    case TI_VAL_BYTES:          return ti_grab(val__sbytes);
    case TI_VAL_REGEX:          return ti_grab(val__sregex);
    case TI_VAL_THING:
        return ti_thing_is_object((ti_thing_t *) val)
                ? ti_grab(val__sthing)
                : (ti_val_t *) ti_thing_type_strv((ti_thing_t *) val);
    case TI_VAL_WRAP:
        return (ti_val_t *) ti_wrap_strv((ti_wrap_t *) val);
    case TI_VAL_ARR:
        return ti_varr_is_list((ti_varr_t *) val)
                ? ti_grab(val__slist)
                : ti_grab(val__stuple);
    case TI_VAL_SET:            return ti_grab(val__sset);
    case TI_VAL_CLOSURE:        return ti_grab(val__sclosure);
    case TI_VAL_ERROR:          return ti_grab(val__serror);
    case TI_VAL_MEMBER:
        return (ti_val_t *) ti_member_enum_get_rname((ti_member_t *) val);
    case TI_VAL_TEMPLATE:
        assert (0);
    }
    assert (0);
    return ti_val_empty_str();
}


/* checks for CLOSURE, ARR, SET */

