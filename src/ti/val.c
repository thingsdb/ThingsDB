/*
 * ti/val.c
 */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ti/closure.h>
#include <ti/nil.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/raw.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
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
static ti_val_t * val__snil;
static ti_val_t * val__strue;
static ti_val_t * val__sfalse;


#define VAL__BUF_SZ 128
static char val__buf[VAL__BUF_SZ];

static ti_val_t * val__unp_map(ti_vup_t * vup, size_t sz, ex_t * e)
{
    mp_obj_t mp_key, mp_val;
    const char * restore_point;
    if (!sz)
        return (ti_val_t *) ti_thing_new_from_unp(vup, sz, e);

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
        if (!mp_may_cast_u64(mp_next(vup->up, &mp_val)))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting an integer value as thing id");
            return NULL;
        }
        return (ti_val_t *) ti_things_thing_o_from_unp(
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
        return (ti_val_t *) ti_things_thing_t_from_unp(vup, e);
    case TI_KIND_C_CLOSURE:
    {
        ti_syntax_t syntax = {
                .val_cache_n = 0,
                .flags = vup->collection
                    ? TI_SYNTAX_FLAG_COLLECTION
                    : TI_SYNTAX_FLAG_THINGSDB,
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
            mp_next(vup->up, &mp_val) != MP_MAP || mp_val.via.sz != 1)
        {
            ex_set(e, EX_BAD_DATA,
                "wrap type must be written according the "
                "following syntax: {\""TI_KIND_S_WRAP"\": [type_id, {...}]");
            return NULL;
        }

        vthing = val__unp_map(vup, 1, e);
        if (!vthing)
            return NULL;

        if (!ti_val_is_thing(vthing))
        {
            ex_set(e, EX_BAD_DATA,
                "wrap type is expecting a wrapped `"TI_VAL_THING_S"` but "
                "got type `%s` instead",
                ti_val_str(vthing));
            ti_val_drop(vthing);
            return NULL;
        }

        wrap = ti_wrap_create((ti_thing_t *) vthing, mp_type_id.via.u64);
        if (!wrap)
            ex_set_mem(e);
        return (ti_val_t *) wrap;
    }
    }

    /* restore the unpack pointer to the first property */
    vup->up->pt = restore_point;
    return (ti_val_t *) ti_thing_new_from_unp(vup, sz, e);
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
    }

    if (vec_push(&varr->vec, val))
        ex_set_mem(e);
    return e->nr;
}

/*
 * Return NULL when failed and `e` is set to an appropriate error message
 */
ti_val_t * ti_val_from_unp_e(ti_vup_t * vup, ex_t * e)
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
            v = ti_val_from_unp_e(vup, e);
            if (!v || val__push(varr, v, e))
            {
                ti_val_drop(v);
                ti_val_drop((ti_val_t *) varr);
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
    val__empty_str = (ti_val_t *) ti_str_from_fmt("");
    val__snil = (ti_val_t *) ti_str_from_fmt("nil");
    val__strue = (ti_val_t *) ti_str_from_fmt("true");
    val__sfalse = (ti_val_t *) ti_str_from_fmt("false");

    if (!val__empty_bin || !val__empty_str || !val__snil || !val__strue ||
        !val__sfalse)
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
    ti_val_drop(val__snil);
    ti_val_drop(val__strue);
    ti_val_drop(val__sfalse);
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
        free(val);
        return;
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
    }
}

int ti_val_make_int(ti_val_t ** val, int64_t i)
{
    ti_val_t * v = (ti_val_t *) ti_vint_create(i);
    if (!v)
        return -1;

    ti_val_drop(*val);
    *val = v;
    return 0;
}

int ti_val_make_float(ti_val_t ** val, double d)
{
    ti_val_t * v = (ti_val_t *) ti_vfloat_create(d);
    if (!v)
        return -1;

    ti_val_drop(*val);
    *val = v;
    return 0;
}

/*
 * Return NULL when failed. Otherwise a new value with a reference.
 */
ti_val_t * ti_val_from_unp(ti_vup_t * vup)
{
    ex_t e = {0};
    ti_val_t * val;

    val = ti_val_from_unp_e(vup, &e);
    if (e.nr)
        log_error("failed to unpack value: %s (%d) ", e.msg, e.nr);
    return val;
}

ti_val_t * ti_val_empty_str(void)
{
    ti_incref(val__empty_str);
    return val__empty_str;
}

ti_val_t * ti_val_empty_bin(void)
{
    ti_incref(val__empty_bin);
    return val__empty_bin;
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
        return &ti()->access_thingsdb;
    case TI_SCOPE_NODE:
        *scope_id = TI_SCOPE_NODE;
        return &ti()->access_node;
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
        const char * s = strx_from_int64((*(ti_vint_t **) val)->int_, &n);
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
        const char * s = strx_from_double((*(ti_vfloat_t **) val)->float_, &n);
        v = (ti_val_t *) ti_str_create(s, n);
        if (!v)
        {
            ex_set_mem(e);
            return e->nr;
        }
        break;
    }
    case TI_VAL_BOOL:
        v = (*(ti_vbool_t **) val)->bool_ ? val__strue : val__sfalse;
        ti_incref(v);
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
        ex_set(e, EX_TYPE_ERROR, "cannot convert type `%s` to `"TI_VAL_STR_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_ERROR:
        v = (ti_val_t *) ti_str_create(
                (*(ti_verror_t **) val)->msg,
                (*(ti_verror_t **) val)->msg_n);
        if (!v)
            return -1;
        break;
        ti_incref(v);
        break;
    }

    ti_val_drop(*val);
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
    case TI_VAL_MP:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        ex_set(e, EX_TYPE_ERROR, "cannot convert type `%s` to `"TI_VAL_BYTES_S"`",
                ti_val_str(*val));
        return e->nr;
    }

    ti_val_drop(*val);
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
        ex_set(e, EX_TYPE_ERROR, "cannot convert type `%s` to `"TI_VAL_INT_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_INT:
        return 0;
    case TI_VAL_FLOAT:
        if (ti_val_overflow_cast((*(ti_vfloat_t **) val)->float_))
            goto overflow;

        i = (int64_t) (*(ti_vfloat_t **) val)->float_;
        break;
    case TI_VAL_BOOL:
        i = (*(ti_vbool_t **) val)->bool_;
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
        ex_set(e, EX_TYPE_ERROR, "cannot convert type `%s` to `"TI_VAL_FLOAT_S"`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_INT:
        d = (double) (*(ti_vint_t **) val)->int_;
        break;
    case TI_VAL_FLOAT:
        return 0;
    case TI_VAL_BOOL:
        d = (double) (*(ti_vbool_t **) val)->bool_;
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
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        ex_set(e, EX_TYPE_ERROR, "cannot convert type `%s` to `"TI_VAL_ARR_S"`",
                ti_val_str(*val));
        break;
    case TI_VAL_ARR:
        break;
    case TI_VAL_SET:
        if (ti_vset_to_list((ti_vset_t **) val))
            ex_set_mem(e);
        break;
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
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_WRAP:
        ex_set(e, EX_TYPE_ERROR, "cannot convert type `%s` to `"TI_VAL_SET_S"`",
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
        vec_t * vec = ((ti_varr_t *) *val)->vec;
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

        ti_val_drop(*val);
        *val = (ti_val_t *) vset;
        break;
    }
    case TI_VAL_SET:
        break;
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
        return !!((ti_vint_t *) val)->int_;
    case TI_VAL_FLOAT:
        return !!((ti_vfloat_t *) val)->float_;
    case TI_VAL_BOOL:
        return ((ti_vbool_t *) val)->bool_;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        return !!((ti_raw_t *) val)->n;
    case TI_VAL_REGEX:
        return true;
    case TI_VAL_ARR:
        return !!((ti_varr_t *) val)->vec->n;
    case TI_VAL_SET:
        return !!((ti_vset_t *) val)->imap->n;
    case TI_VAL_THING:
        return !!((ti_thing_t *) val)->items->n;
    case TI_VAL_WRAP:
        return !!((ti_wrap_t *) val)->thing->items->n;
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        return true;
    }
    assert (0);
    return false;
}

_Bool ti_val_is_valid_name(ti_val_t * val)
{
    return  val->tp == TI_VAL_NAME || (
            val->tp == TI_VAL_STR && ti_name_is_valid_strn(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n));
}

/*
 * Can only be called on values which have a length.
 */
size_t ti_val_get_len(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_MP:
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
        return ((ti_varr_t *) val)->vec->n;
    case TI_VAL_SET:
        return ((ti_vset_t *) val)->imap->n;
    case TI_VAL_CLOSURE:
        break;
    case TI_VAL_ERROR:
        return ((ti_verror_t *) val)->msg_n;
    }
    assert (0);
    return 0;
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
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
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
            for (vec_each(((ti_varr_t *) val)->vec, ti_val_t, v))
                if (ti_val_gen_ids(v))
                    return -1;
        break;
    case TI_VAL_SET:
        if (((ti_vset_t *) val)->imap->n)
        {
            vec_t * vec = imap_vec(((ti_vset_t *) val)->imap);
            if (!vec)
                return -1;

            for (vec_each(vec, ti_thing_t, thing))
                if (!thing->id && ti_thing_gen_id(thing))
                    return -1;
        }
        break;
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        break;
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
    case TI_VAL_MP:             return TI_VAL_INFO_S;
    case TI_VAL_NAME:
    case TI_VAL_STR:            return TI_VAL_STR_S;
    case TI_VAL_BYTES:            return TI_VAL_BYTES_S;
    case TI_VAL_REGEX:          return TI_VAL_REGEX_S;
    case TI_VAL_THING:          return ti_thing_is_object((ti_thing_t *) val)
                                    ? TI_VAL_THING_S
                                    : ti_thing_type_str((ti_thing_t *) val);
    case TI_VAL_WRAP:           return ti_wrap_str((ti_wrap_t *) val);
    case TI_VAL_ARR:            return ti_varr_is_list((ti_varr_t *) val)
                                    ? TI_VAL_ARR_LIST_S
                                    : TI_VAL_ARR_TUPLE_S;
    case TI_VAL_SET:            return TI_VAL_SET_S;
    case TI_VAL_CLOSURE:        return TI_VAL_CLOSURE_S;
    case TI_VAL_ERROR:          return TI_VAL_ERROR_S;
    }
    assert (0);
    return "unknown";
}

/* checks for CLOSURE, ARR, SET */

