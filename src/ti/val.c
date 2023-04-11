/*
 * ti/val.c
 */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/closure.h>
#include <ti/collection.inline.h>
#include <ti/datetime.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/future.h>
#include <ti/future.inline.h>
#include <ti/member.h>
#include <ti/member.inline.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/raw.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
#include <ti/room.h>
#include <ti/room.inline.h>
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
#include <ti/vtask.inline.h>
#include <ti/wrap.inline.h>
#include <tiinc.h>
#include <util/logger.h>
#include <util/strx.h>

#define VAL__CMP(__s) ti_raw_eq_strn((*(ti_raw_t **) val), __s, strlen(__s))

static ti_val_t * val__empty_bin;
static ti_val_t * val__empty_str;
static ti_val_t * val__default_re;
static ti_val_t * val__default_closure;
static ti_val_t * val__sbool;
static ti_val_t * val__sbytes;
static ti_val_t * val__sclosure;
static ti_val_t * val__sdatetime;
static ti_val_t * val__serror;
static ti_val_t * val__sfloat;
static ti_val_t * val__sfuture;
static ti_val_t * val__sint;
static ti_val_t * val__slist;
static ti_val_t * val__smpdata;
static ti_val_t * val__sregex;
static ti_val_t * val__sroom;
static ti_val_t * val__stask;
static ti_val_t * val__sset;
static ti_val_t * val__sstr;
static ti_val_t * val__sthing;
static ti_val_t * val__stimeval;
static ti_val_t * val__stuple;
static ti_val_t * val__swthing;
static ti_val_t * val__tar_gz_str;
static ti_val_t * val__gs_str;
static ti_val_t * val__charset_str;

/* name */
ti_val_t * val__year_name;
ti_val_t * val__month_name;
ti_val_t * val__day_name;
ti_val_t * val__hour_name;
ti_val_t * val__minute_name;
ti_val_t * val__second_name;
ti_val_t * val__gmt_offset_name;
ti_val_t * val__module_name;
ti_val_t * val__deep_name;
ti_val_t * val__flags_name;
ti_val_t * val__load_name;
ti_val_t * val__beautify_name;
ti_val_t * val__parent_name;
ti_val_t * val__parent_type_name;
ti_val_t * val__key_name;
ti_val_t * val__key_type_name;

/* string */
ti_val_t * val__sany;
ti_val_t * val__snil;
ti_val_t * val__strue;
ti_val_t * val__sfalse;

#define VAL__BUF_SZ 128
static char val__buf[VAL__BUF_SZ];

static ti_val_t * val__unp_map(ti_vup_t * vup, size_t sz, ex_t * e)
{
    mp_obj_t mp_key, mp_val;

    if (mp_next(vup->up, &mp_key) != MP_STR || mp_key.via.str.n != 1)
    {
        ex_set(e, EX_TYPE_ERROR, "expecting a reserved key");
        return NULL;
    }

    switch ((ti_val_kind) *mp_key.via.str.data)
    {
    case TI_KIND_C_INSTANCE:
        if (!vup->collection)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot unpack a `typed thing` without a collection");
            return NULL;
        }
        return (ti_val_t *) ti_things_thing_t_from_vup(vup, e);

    case TI_KIND_C_OBJECT:
        if (!vup->collection)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot unpack a `non-typed thing` without a collection");
            return NULL;
        }
        return (ti_val_t *) ti_things_thing_o_from_vup(vup, e);

    case TI_KIND_C_SET:
    {
        ti_val_t * vthing;
        ti_vset_t * vset = ti_vset_create();
        size_t i;

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

        for (i = mp_val.via.sz; i--;)
        {
            vthing = ti_val_from_vup_e(vup, e);
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
        mp_obj_t obj, mp_msg, mp_code;

        if (sz != 1 ||
            mp_next(vup->up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(vup->up, &mp_msg) != MP_STR ||
            mp_next(vup->up, &mp_code) != MP_I64 ||
            mp_msg.via.str.n > EX_MAX_SZ ||
            mp_code.via.i64 < EX_MIN_ERR ||
            mp_code.via.i64 > 0)
        {
            /*
             * TODO (COMPAT) Compatibility with v0.x
             */
            if (sz != 3 ||
                mp_skip(vup->up) != MP_STR ||       /* definition */
                mp_skip(vup->up) != MP_STR ||       /* key: error_msg */
                mp_next(vup->up, &mp_msg) != MP_STR ||
                mp_skip(vup->up) != MP_STR ||       /* error_code */
                mp_next(vup->up, &mp_code) != MP_I64 ||
                mp_msg.via.str.n > EX_MAX_SZ ||
                mp_code.via.i64 < EX_MIN_ERR ||
                mp_code.via.i64 > 0)
            {
                ex_set(e, EX_BAD_DATA,
                        "errors must be written according the "
                        "following syntax: {"
                        "\""TI_KIND_S_ERROR"\": \"..err()\","
                        "\"error_msg\": \"..msg\","
                        "\"error_code\": code}");
                return NULL;
            }
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
            mp_next(vup->up, &mp_type_id) != MP_U64)
        {
            ex_set(e, EX_BAD_DATA,
                "wrap type must be written according the "
                "following syntax: {\""TI_KIND_S_WRAP"\": [type_id, {...}]");
            return NULL;
        }

        vthing = ti_val_from_vup_e(vup, e);
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
    case TI_KIND_C_TIMEVAL:
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
                "invalid offset: %"PRId64, mp_offset.via.i64);
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
                    "invalid time zone, expecting `nil` or `u64`");
            return NULL;
        }

        dt = ((ti_val_kind) *mp_key.via.str.data) == TI_KIND_C_TIMEVAL
                ? ti_timeval_from_i64(mp_ts.via.i64, mp_offset.via.i64, tz)
                : ti_datetime_from_i64(mp_ts.via.i64, mp_offset.via.i64, tz);

        if (!dt)
            ex_set_mem(e);

        return (ti_val_t *) dt;
    }
    /*
     * TODO (COMPAT) For compatibility with data from before v1.1.1
     */
    case TI_KIND_C_THING_OBSOLETE_:
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
        return (ti_val_t *) ti_things_thing_o_from_vup__deprecated(
                vup,
                mp_val.via.u64,
                sz,
                e);
    /*
     * TODO (COMPAT) For compatibility with data from v0.x
     */
    case TI_KIND_C_CLOSURE_OBSOLETE_:
    {
        ti_qbind_t syntax = {
                .immutable_n = 0,
                .flags = vup->collection
                    ? TI_QBIND_FLAG_COLLECTION
                    : TI_QBIND_FLAG_THINGSDB,
        };
        if (sz != 1 || mp_next(vup->up, &mp_val) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "closures must be written according the following "
                    "syntax: {\""TI_KIND_S_CLOSURE_OBSOLETE_"\": \"...\"");
            return NULL;
        }
        return (ti_val_t *) ti_closure_from_strn(
                &syntax,
                mp_val.via.str.data,
                mp_val.via.str.n, e);
    }
    /*
     * TODO (COMPAT) For compatibility with data from v0.x
     */
    case TI_KIND_C_REGEX_OBSOLETE_:
    {
        if (sz != 1 || mp_next(vup->up, &mp_val) != MP_STR)        {
            ex_set(e, EX_BAD_DATA,
                    "regular expressions must be written according the "
                    "following syntax: {\""TI_KIND_S_REGEX_OBSOLETE_"\": \"...\"");
            return NULL;
        }

        return (ti_val_t *) ti_regex_from_strn(
                mp_val.via.str.data,
                mp_val.via.str.n, e);
    }
    }

    ex_set(e, EX_VALUE_ERROR,
            "unsupported reserved key `%c`",
            *mp_key.via.str.data);

    return NULL;
}

/*
 * Does not increment the `val` reference counter.
 */
static int val__push(ti_varr_t * varr, ti_val_t * val, ex_t * e)
{
    assert (ti_varr_is_list(varr));
    /*
     * Futures can never occur at this point since they are never packed;
     */
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:
    case TI_VAL_WRAP:
        varr->flags |= TI_VARR_FLAG_MHT;
        break;
    case TI_VAL_ROOM:
        varr->flags |= TI_VARR_FLAG_MHR;
        break;
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_TASK:
    case TI_VAL_ERROR:
    case TI_VAL_MPDATA:
    case TI_VAL_CLOSURE:
        break;
    case TI_VAL_ARR:
    {
        /* Make sure the arr is converted to a `tuple` and copy the
         * may-have-* flags to the parent.
         */
        ti_varr_t * arr = (ti_varr_t *) val;
        arr->flags |= TI_VARR_FLAG_TUPLE;
        ti_varr_set_may_flags(varr, arr);
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
            varr->flags |= TI_VARR_FLAG_MHT;
        break;
    case TI_VAL_FUTURE:
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
        return vup->isclient
                ? (ti_val_t *) ti_thing_new_from_vup(vup, obj.via.sz, e)
                : val__unp_map(vup, obj.via.sz, e);
    case MP_EXT:
    {
        switch((mpack_ext_t) obj.via.ext.tp)
        {
        case MPACK_EXT_MPACK:
        {
            ti_raw_t * raw = ti_mp_create(obj.via.ext.data, obj.via.ext.n);
            if (!raw)
                ex_set_mem(e);
            return (ti_val_t *) raw;
        }
        case MPACK_EXT_REGEX:
            return (ti_val_t *) ti_regex_from_strn(
                    (const char *) obj.via.ext.data,
                    obj.via.ext.n,
                    e);
        case MPACK_EXT_CLOSURE:
        {
            ti_qbind_t syntax = {
                    .immutable_n = 0,
                    .flags = vup->collection
                        ? TI_QBIND_FLAG_COLLECTION
                        : TI_QBIND_FLAG_THINGSDB,
            };
            return (ti_val_t *) ti_closure_from_strn(
                    &syntax,
                    (const char *) obj.via.ext.data,
                    obj.via.ext.n,
                    e);
        }
        case MPACK_EXT_ROOM:
        {
            ti_room_t * room;
            uint64_t room_id;
            if (!vup->collection)
            {
                ex_set(e, EX_BAD_DATA,
                        "cannot unpack a `room` without a collection");
                return NULL;
            }
            if (obj.via.ext.n != sizeof(uint64_t))
            {
                ex_set(e, EX_BAD_DATA,
                        "expecting a room type of size %zu but got %"PRIu32,
                        sizeof(uint64_t), obj.via.ext.n);
                return NULL;
            }

            mp_read_uint64(obj.via.ext.data, &room_id);
            room = ti_collection_room_by_id(vup->collection, room_id);
            if (!room)
            {
                room = ti_room_create(room_id, vup->collection);
                if (!room || imap_add(vup->collection->rooms, room_id, room))
                {
                    ti_val_drop((ti_val_t *) room);
                    ex_set_mem(e);
                }
                /* update the next free id if required */
                ti_update_next_free_id(room_id);
            }
            else
                ti_incref(room);

            return (ti_val_t *) room;
        }
        case MPACK_EXT_TASK:
        {
            uint64_t task_id;
            vec_t * vtasks = vup->collection
                    ? vup->collection->vtasks
                    : ti.tasks->vtasks;

            if (obj.via.ext.n != sizeof(uint64_t))
            {
                ex_set(e, EX_BAD_DATA,
                        "expecting a task type of size %zu but got %"PRIu32,
                        sizeof(uint64_t), obj.via.ext.n);
                return NULL;
            }

            mp_read_uint64(obj.via.ext.data, &task_id);

            if (task_id) for (vec_each(vtasks, ti_vtask_t, vtask))
            {
                if (vtask->id == task_id)
                {
                    ti_incref(vtask);
                    return (ti_val_t *) vtask;
                }
            }
            return (ti_val_t *) ti_vtask_nil();
        }
        case MPACK_EXT_THING:
        {
            uint64_t thing_id;
            ti_thing_t * thing;

            if (obj.via.ext.n != sizeof(uint64_t))
            {
                ex_set(e, EX_BAD_DATA,
                        "expecting a thing id of size %zu but got %"PRIu32,
                        sizeof(uint64_t), obj.via.ext.n);
                return NULL;
            }

            if (!vup->collection)
            {
                ex_set(e, EX_BAD_DATA,
                        "cannot unpack a `thing` without a collection");
                return NULL;
            }

            mp_read_uint64(obj.via.ext.data, &thing_id);

            thing = ti_collection_find_thing(vup->collection, thing_id);

            if (thing)
                ti_incref(thing);
            else
                ex_set(e, EX_LOOKUP_ERROR,
                    "cannot unpack find `thing` with id %"PRIu64, thing_id);

            return (ti_val_t *) thing;
        }
        }
        ex_set(e, EX_BAD_DATA,
                "msgpack extension type %d is not supported by ThingsDB",
                obj.via.ext.tp);
        return NULL;
    }
    }

    ex_set(e, EX_BAD_DATA, "unexpected code reached while unpacking value");
    return NULL;
}

int ti_val_init_common(void)
{
    ex_t e = {0};
    ti_qbind_t syntax = {
            .immutable_n = 0,
            .flags = TI_QBIND_FLAG_COLLECTION|TI_QBIND_FLAG_THINGSDB,
    };
    val__default_closure = \
            (ti_val_t *) ti_closure_from_strn(&syntax, "||nil", 5, &e);
    val__empty_bin = (ti_val_t *) ti_bin_create(NULL, 0);
    val__empty_str = (ti_val_t *) ti_str_from_str("");
    val__default_re = (ti_val_t *) ti_regex_from_strn("/.*/", 4, &e);
    val__sany = (ti_val_t *) ti_str_from_str("any");
    val__snil = (ti_val_t *) ti_str_from_str(TI_VAL_NIL_S);
    val__strue = (ti_val_t *) ti_str_from_str("true");
    val__sfalse = (ti_val_t *) ti_str_from_str("false");
    val__sbool = (ti_val_t *) ti_str_from_str(TI_VAL_BOOL_S);
    val__sdatetime = (ti_val_t *) ti_str_from_str(TI_VAL_DATETIME_S);
    val__stimeval = (ti_val_t *) ti_str_from_str(TI_VAL_TIMEVAL_S);
    val__sint = (ti_val_t *) ti_str_from_str(TI_VAL_INT_S);
    val__sfloat = (ti_val_t *) ti_str_from_str(TI_VAL_FLOAT_S);
    val__sstr = (ti_val_t *) ti_str_from_str(TI_VAL_STR_S);
    val__sbytes = (ti_val_t *) ti_str_from_str(TI_VAL_BYTES_S);
    val__smpdata = (ti_val_t *) ti_str_from_str(TI_VAL_MPDATA_S);
    val__sregex = (ti_val_t *) ti_str_from_str(TI_VAL_REGEX_S);
    val__sroom = (ti_val_t *) ti_str_from_str(TI_VAL_ROOM_S);
    val__stask = (ti_val_t *) ti_str_from_str(TI_VAL_TASK_S);
    val__serror = (ti_val_t *) ti_str_from_str(TI_VAL_ERROR_S);
    val__sclosure = (ti_val_t *) ti_str_from_str(TI_VAL_CLOSURE_S);
    val__sfuture = (ti_val_t *) ti_str_from_str(TI_VAL_FUTURE_S);
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
    val__module_name = (ti_val_t *) ti_names_from_str("module");
    val__deep_name = (ti_val_t *) ti_names_from_str("deep");
    val__flags_name = (ti_val_t *) ti_names_from_str("flags");
    val__load_name = (ti_val_t *) ti_names_from_str("load");
    val__beautify_name = (ti_val_t *) ti_names_from_str("beautify");
    val__parent_name = (ti_val_t *) ti_names_from_str("parent");
    val__parent_type_name = (ti_val_t *) ti_names_from_str("parent_type");
    val__key_name = (ti_val_t *) ti_names_from_str("key");
    val__key_type_name = (ti_val_t *) ti_names_from_str("key_type");


    if (!val__empty_bin || !val__empty_str || !val__snil || !val__strue ||
        !val__sfalse || !val__sbool || !val__sdatetime || !val__stimeval ||
        !val__sint || !val__sfloat || !val__sstr || !val__sbytes ||
        !val__smpdata || !val__sregex || !val__serror || !val__sclosure ||
        !val__slist || !val__stuple || !val__sset || !val__sthing ||
        !val__swthing || !val__tar_gz_str || !val__sany || !val__gs_str ||
        !val__charset_str || !val__default_re || !val__default_closure ||
        !val__sroom || !val__stask || !val__year_name || !val__month_name ||
        !val__day_name || !val__hour_name || !val__minute_name ||
        !val__second_name || !val__gmt_offset_name || !val__sfuture ||
        !val__module_name || !val__deep_name || !val__load_name ||
        !val__beautify_name || !val__parent_name || !val__parent_type_name ||
        !val__key_name || !val__key_type_name || !val__flags_name)
    {
        return -1;
    }
    return 0;
}

void ti_val_drop_common(void)
{
    ti_val_drop(val__empty_bin);
    ti_val_drop(val__empty_str);
    ti_val_drop(val__default_re);
    ti_val_drop(val__default_closure);
    ti_val_drop(val__sany);
    ti_val_drop(val__snil);
    ti_val_drop(val__strue);
    ti_val_drop(val__sfalse);
    ti_val_drop(val__sbool);
    ti_val_drop(val__sdatetime);
    ti_val_drop(val__stimeval);
    ti_val_drop(val__sint);
    ti_val_drop(val__sfloat);
    ti_val_drop(val__sstr);
    ti_val_drop(val__sbytes);
    ti_val_drop(val__smpdata);
    ti_val_drop(val__sregex);
    ti_val_drop(val__sroom);
    ti_val_drop(val__stask);
    ti_val_drop(val__serror);
    ti_val_drop(val__sclosure);
    ti_val_drop(val__sfuture);
    ti_val_drop(val__slist);
    ti_val_drop(val__stuple);
    ti_val_drop(val__sset);
    ti_val_drop(val__sthing);
    ti_val_drop(val__swthing);
    ti_val_drop(val__tar_gz_str);
    ti_val_drop(val__gs_str);
    ti_val_drop(val__charset_str);
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

ti_val_t * ti_val_default_re(void)
{
    ti_incref(val__default_re);
    return val__default_re;
}

ti_val_t * ti_val_default_closure(void)
{
    ti_incref(val__default_closure);
    return val__default_closure;
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

ti_val_t * ti_val_wrapped_thing_str(void)
{
    ti_incref(val__swthing);
    return val__swthing;
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
                scope.via.collection_name.sz,
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
    case TI_VAL_MPDATA:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_FUTURE:
    case TI_VAL_ERROR:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_BYTES_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_MEMBER:
        v = VMEMBER(*val);
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
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_FUTURE:
    case TI_VAL_MPDATA:
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
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_FUTURE:
    case TI_VAL_MPDATA:
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
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_FUTURE:
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
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_FUTURE:
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
    case TI_VAL_MPDATA:
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
        return !!ti_thing_n((ti_thing_t *) val);
    case TI_VAL_WRAP:
        return !!ti_thing_n(((ti_wrap_t *) val)->thing);
    case TI_VAL_ROOM:
        return ((ti_room_t *) val)->id;
    case TI_VAL_TASK:
        return ((ti_vtask_t *) val)->run_at;
    case TI_VAL_CLOSURE:
    case TI_VAL_FUTURE:
        return true;
    case TI_VAL_ERROR:
        return false;
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
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_DATETIME:
    case TI_VAL_TEMPLATE:
        break;
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_MPDATA:
        return ((ti_raw_t *) val)->n;
    case TI_VAL_REGEX:
        break;
    case TI_VAL_THING:
        return ti_thing_n((ti_thing_t *) val);
    case TI_VAL_WRAP:
        return ti_thing_n(((ti_wrap_t *) val)->thing);
    case TI_VAL_ARR:
        return VARR(val)->n;
    case TI_VAL_SET:
        return VSET(val)->n;
    case TI_VAL_CLOSURE:
        break;
    case TI_VAL_MEMBER:
        return ti_val_get_len(VMEMBER(val));
    case TI_VAL_FUTURE:
    case TI_VAL_ERROR:
        break;
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
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_TASK:
    case TI_VAL_MEMBER:
        /* enum can be skipped; even a thing on an enum is guaranteed to
         * have an ID since they are triggered when creating or changing the
         * enumerator.
         *
         * task can be skipped as id's for task are created immediately on
         * the creation of a task.
         */
        break;

    case TI_VAL_THING:
        if (!((ti_thing_t *) val)->id)
            return ti_thing_gen_id((ti_thing_t *) val);
        /*
         * New things 'under' an existing thing will get their own task,
         * so here we do not need recursion.
         */
        break;
    case TI_VAL_WRAP:
        if (!((ti_wrap_t *) val)->thing->id)
            return ti_thing_gen_id(((ti_wrap_t *) val)->thing);
        /*
         * New things 'under' an existing thing will get their own task,
         * so here we do not need recursion.
         */
        break;
    case TI_VAL_ROOM:
        if (!((ti_room_t *) val)->id)
            return ti_room_gen_id((ti_room_t *) val);
        break;
    case TI_VAL_ARR:
        /*
         * Here the code really benefits from the `may-have-things` flag since
         * must attached arrays will contain "only" things, or no things.
         */
        if (ti_varr_may_gen_ids((ti_varr_t *) val))
            for (vec_each(VARR(val), ti_val_t, v))
                if (ti_val_gen_ids(v))
                    return -1;
        break;
    case TI_VAL_SET:
        return imap_walk(VSET(val), (imap_cb) val__walk_set, NULL);
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        break;
    case TI_VAL_FUTURE:
    case TI_VAL_TEMPLATE:
        assert (0);
    }
    return 0;
}

size_t ti_val_alloc_size(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_ROOM:
        return 64;
    case TI_VAL_TASK:
    case TI_VAL_DATETIME:
        return 128;
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        return ((ti_raw_t *) val)->n + 32;
    case TI_VAL_REGEX:
        return ((ti_regex_t *) val)->pattern->n + 32;
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
        return 65536;
    case TI_VAL_CLOSURE:
        return 4096;
    case TI_VAL_ERROR:
        return ((ti_verror_t *) val)->msg_n + 128;
    case TI_VAL_MEMBER:
        return ti_val_alloc_size(VMEMBER(val));
    case TI_VAL_FUTURE:
        return VFUT(val) ? ti_val_alloc_size(VFUT(val)) : 64;
    case TI_VAL_TEMPLATE:
        assert (0);
    }
    assert (0);
    return 0;
}

ti_val_t * ti_val_strv(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:            return ti_grab(val__snil);
    case TI_VAL_INT:            return ti_grab(val__sint);
    case TI_VAL_FLOAT:          return ti_grab(val__sfloat);
    case TI_VAL_BOOL:           return ti_grab(val__sbool);
    case TI_VAL_DATETIME:
        return ti_datetime_is_timeval((ti_datetime_t *) val)
                ? ti_grab(val__stimeval)
                : ti_grab(val__sdatetime);
    case TI_VAL_MPDATA:         return ti_grab(val__smpdata);
    case TI_VAL_NAME:
    case TI_VAL_STR:            return ti_grab(val__sstr);
    case TI_VAL_BYTES:          return ti_grab(val__sbytes);
    case TI_VAL_REGEX:          return ti_grab(val__sregex);
    case TI_VAL_THING:
        return ti_thing_is_object((ti_thing_t *) val)
                ? ti_grab(val__sthing)
                : (ti_val_t *) ti_thing_type_strv((ti_thing_t *) val);
    case TI_VAL_WRAP:
        return (ti_val_t *) ti_wrap_type_strv((ti_wrap_t *) val);
    case TI_VAL_ROOM:           return ti_grab(val__sroom);
    case TI_VAL_TASK:           return ti_grab(val__stask);
    case TI_VAL_ARR:
        return ti_varr_is_list((ti_varr_t *) val)
                ? ti_grab(val__slist)
                : ti_grab(val__stuple);
    case TI_VAL_SET:            return ti_grab(val__sset);
    case TI_VAL_CLOSURE:        return ti_grab(val__sclosure);
    case TI_VAL_ERROR:          return ti_grab(val__serror);
    case TI_VAL_MEMBER:
        return (ti_val_t *) ti_member_enum_get_rname((ti_member_t *) val);
    case TI_VAL_FUTURE:         return ti_grab(val__sfuture);
    case TI_VAL_TEMPLATE:
        assert (0);
    }
    assert (0);
    return ti_val_empty_str();
}

int ti_val_copy(ti_val_t ** val, ti_thing_t * parent, void * key, uint8_t deep)
{
    if (!deep)
    {
        ex_t e = {0};
        return ti_val_make_assignable(val, parent, key, &e);
    }

    switch ((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_TASK:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
        return 0;
    case TI_VAL_THING:
        return ti_thing_copy((ti_thing_t **) val, deep);
    case TI_VAL_WRAP:
        return ti_wrap_copy((ti_wrap_t **) val, deep);
    case TI_VAL_ROOM:
        return ti_room_copy((ti_room_t **) val);  /* copy a room */
    case TI_VAL_ARR:
        if (ti_varr_copy((ti_varr_t **) val, deep))
            return -1;
        ((ti_varr_t *) *val)->parent = parent;
        ((ti_varr_t *) *val)->key_ = key;
        return 0;
    case TI_VAL_SET:
        if (ti_vset_copy((ti_vset_t **) val, deep))
            return -1;
        ((ti_vset_t *) *val)->parent = parent;
        ((ti_vset_t *) *val)->key_ = key;
        return 0;
    case TI_VAL_CLOSURE:
    {
        ex_t e = {0};
        return ti_closure_unbound((ti_closure_t * ) *val, &e);
    }
    case TI_VAL_FUTURE:
        ti_val_unsafe_drop(*val);
        *val = (ti_val_t *) ti_nil_get();
        return 0;
    case TI_VAL_TEMPLATE:
        break;
    }
    assert(0);
    return -1;
}

int ti_val_dup(ti_val_t ** val, ti_thing_t * parent, void * key, uint8_t deep)
{
    if (!deep)
    {
        ex_t e = {0};
        return ti_val_make_assignable(val, parent, key, &e);
    }

    switch ((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_TASK:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
        return 0;
    case TI_VAL_THING:
        return ti_thing_dup((ti_thing_t **) val, deep);
    case TI_VAL_WRAP:
        return ti_wrap_dup((ti_wrap_t **) val, deep);
    case TI_VAL_ROOM:
        return ti_room_copy((ti_room_t **) val);  /* copy a room */
    case TI_VAL_ARR:
        if (ti_varr_dup((ti_varr_t **) val, deep))
            return -1;
        ((ti_varr_t *) *val)->parent = parent;
        ((ti_varr_t *) *val)->key_ = key;
        return 0;
    case TI_VAL_SET:
        if (ti_vset_dup((ti_vset_t **) val, deep))
            return -1;
        ((ti_vset_t *) *val)->parent = parent;
        ((ti_vset_t *) *val)->key_ = key;
        return 0;
    case TI_VAL_CLOSURE:
    {
        ex_t e = {0};
        return ti_closure_unbound((ti_closure_t * ) *val, &e);
    }
    case TI_VAL_FUTURE:
        ti_val_unsafe_drop(*val);
        *val = (ti_val_t *) ti_nil_get();
        return 0;
    case TI_VAL_TEMPLATE:
        break;
    }
    assert(0);
    return -1;
}

/*
 * `to_str` functions
 */
int ti_val_nil_to_str(ti_val_t ** val, ex_t * UNUSED(e))
{
    ti_decref(*val);
    ti_incref(val__snil);
    *val = val__snil;
    return 0;
}
int ti_val_int_to_str(ti_val_t ** val, ex_t * e)
{
    size_t n;
    const char * s = strx_from_int64(VINT(*val), &n);
    ti_val_t * v = (ti_val_t *) ti_str_create(s, n);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_float_to_str(ti_val_t ** val, ex_t * e)
{
    size_t n;
    const char * s = strx_from_double(VFLOAT(*val), &n);
    ti_val_t * v = (ti_val_t *) ti_str_create(s, n);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_bool_to_str(ti_val_t ** val, ex_t * UNUSED(e))
{
    ti_val_t * v = VBOOL(*val) ? val__strue : val__sfalse;
    ti_decref(*val);
    ti_incref(v);
    *val = v;
    return 0;
}
int ti_val_datetime_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = (ti_val_t *) ti_datetime_to_str((ti_datetime_t *) *val, e);
    if (!v)
        return e->nr;
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_bytes_to_str(ti_val_t ** val, ex_t * e)
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
        return 0;
    }
    r = ti_str_create((const char *) r->data, r->n);
    if (!r)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = (ti_val_t *) r;
    return 0;
}
int ti_val_regex_to_str(ti_val_t ** val, ex_t * UNUSED(e))
{
    ti_val_t * v = (ti_val_t *) (*(ti_regex_t **) val)->pattern;
    ti_val_unsafe_drop(*val);
    ti_incref(v);
    *val = v;
    return 0;
}
int ti_val_thing_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = (ti_val_t *) ti_thing_str((ti_thing_t *) *val);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_wrap_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = (ti_val_t *) ti_wrap_str((ti_wrap_t *) *val);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_room_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = (ti_val_t *) ti_room_str((ti_room_t *) *val);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_vtask_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = (ti_val_t *) ti_vtask_str((ti_vtask_t *) *val);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_error_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = (ti_val_t *) ti_str_create(
            (*(ti_verror_t **) val)->msg,
            (*(ti_verror_t **) val)->msg_n);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_member_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = VMEMBER(*val);
    ti_incref(v);
    if (ti_val(v)->to_str(&v, e))  /* bug #344 */
    {
        ti_decref(v);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
int ti_val_closure_to_str(ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = (ti_val_t *) ti_closure_def((ti_closure_t *) *val);
    if (!v)
    {
        ex_set_mem(e);
        return e->nr;
    }
    ti_val_unsafe_drop(*val);
    *val = v;
    return 0;
}
