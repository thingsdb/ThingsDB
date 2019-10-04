/*
 * ti/val.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti/wrap.inline.h>
#include <ti/closure.h>
#include <tiinc.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/regex.h>
#include <ti/nil.h>
#include <ti/things.h>
#include <ti/val.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/verror.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <util/logger.h>
#include <util/strx.h>
#include <math.h>
#include <ti/thing.inline.h>

#define VAL__CMP(__s) ti_raw_eq_strn((*(ti_raw_t **) val), __s, strlen(__s))

static ti_val_t * val__sempty;
static ti_val_t * val__snil;
static ti_val_t * val__strue;
static ti_val_t * val__sfalse;
static ti_val_t * val__sblob;
static ti_val_t * val__sarray;
static ti_val_t * val__sset;
static ti_val_t * val__sthing;
static ti_val_t * val__sclosure;
static ti_val_t * val__swrap;

#define VAL__BUF_SZ 128
static char val__buf[VAL__BUF_SZ];

static ti_val_t * val__unp_map(
        qp_unpacker_t * unp,
        ti_collection_t * collection,
        ssize_t sz,
        ex_t * e)
{
    qp_obj_t qp_kind, qp_tmp;
    const unsigned char * restore_point;
    if (!sz)
        return (ti_val_t *) ti_thing_new_from_unp(unp, collection, sz, e);

    restore_point = unp->pt;

    if (!qp_is_raw(qp_next(unp, &qp_kind)) || !qp_kind.len)
    {
        ex_set(e, EX_TYPE_ERROR,
                "property names must be of type `raw` "
                "and follow the naming rules"DOC_NAMES);
        return NULL;
    }

    switch ((ti_val_kind) *qp_kind.via.raw)
    {
    case TI_KIND_C_THING:
        if (!collection)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot unpack a `thing` without a collection");
            return NULL;
        }
        if (!qp_is_int(qp_next(unp, &qp_tmp)))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting an integer value as thing id");
            return NULL;
        }
        return (ti_val_t *) ti_things_thing_o_from_unp(
                collection,
                (uint64_t) qp_tmp.via.int64,
                unp,
                sz,
                e);
    case TI_KIND_C_INSTANCE:
        if (!collection)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot unpack a `thing` without a collection");
            return NULL;
        }
        return (ti_val_t *) ti_things_thing_t_from_unp(collection, unp, e);
    case TI_KIND_C_CLOSURE:
    {
        ti_syntax_t syntax = {
                .val_cache_n = 0,
                .flags = collection
                    ? TI_SYNTAX_FLAG_COLLECTION
                    : TI_SYNTAX_FLAG_THINGSDB,
        };

        if (sz != 1 || !qp_is_raw(qp_next(unp, &qp_tmp)))
        {
            ex_set(e, EX_BAD_DATA,
                    "closures must be written according the following syntax: "
                    "{\""TI_KIND_S_CLOSURE"\": \"...\"");
            return NULL;
        }

        return (ti_val_t *) ti_closure_from_strn(
                &syntax,
                (char *) qp_tmp.via.raw,
                qp_tmp.len, e);
    }
    case TI_KIND_C_REGEX:
    {
        if (sz != 1 || !qp_is_raw(qp_next(unp, &qp_tmp)))
        {
            ex_set(e, EX_BAD_DATA,
                    "regular expressions must be written according the "
                    "following syntax: {\""TI_KIND_S_REGEX"\": \"...\"");
            return NULL;
        }

        return (ti_val_t *) ti_regex_from_strn(
                (const char *) qp_tmp.via.raw,
                qp_tmp.len,
                e);
    }
    case TI_KIND_C_SET:
    {
        ti_val_t * vthing;
        ssize_t tsz, arrsz = qp_next(unp, NULL);
        ti_vset_t * vset = ti_vset_create();
        if (!vset)
        {
            ex_set_mem(e);
            return NULL;
        }
        if (sz != 1 || !qp_is_array(arrsz))
        {
            ex_set(e, EX_BAD_DATA,
                    "sets must be written according the "
                    "following syntax: {\""TI_KIND_S_SET"\": [...]");
            return NULL;
        }
        arrsz = arrsz == QP_ARRAY_OPEN ? -1 : arrsz - QP_ARRAY0;

        while (arrsz--)
        {
            tsz = qp_next(unp, &qp_tmp);

            if (qp_is_close(tsz))
                break;

            if (!qp_is_map(tsz))
            {
                ti_vset_destroy(vset);
                ex_set(e, EX_TYPE_ERROR, "sets can only contain things");
                return NULL;
            }

            tsz = tsz == QP_MAP_OPEN ? -1 : tsz - QP_MAP0;
            vthing = val__unp_map(unp, collection, tsz, e);

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
        qp_obj_t qp_msg, qp_code;
        if (    sz != 3 ||
                !qp_is_raw(qp_next(unp, NULL)) ||       /* definition */
                !qp_is_raw(qp_next(unp, NULL)) ||       /* error_msg */
                !qp_is_raw(qp_next(unp, &qp_msg)) ||
                qp_msg.len > EX_MAX_SZ ||
                !qp_is_raw(qp_next(unp, NULL)) ||       /* error_code */
                !qp_is_int(qp_next(unp, &qp_code)) ||
                qp_code.via.int64 < EX_MIN_ERR ||
                qp_code.via.int64 > EX_MAX_BUILD_IN_ERR)
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
                (const char *) qp_msg.via.raw,
                qp_msg.len,
                (int8_t) qp_code.via.int64);

        if (!verror)
            ex_set_mem(e);

        return (ti_val_t *) verror;
    }
    case TI_KIND_C_INFO:
    {
        const uchar * start;
        ti_val_t * qpinfo;
        if (unp->flags & TI_VAL_UNP_FROM_CLIENT)
        {
            ex_set(e, EX_BAD_DATA, "type `info` is not allowed as user input");
            return NULL;
        }
        if (qp_kind.len != 1)
        {
            ex_set(e, EX_BAD_DATA, "invalid info key");
            return NULL;
        }
        unp->pt = restore_point;    /* restore to the start the map */
        --unp->pt;          /* decrease one to restore to the map itself */
        assert (qp_is_map(*unp->pt));
        start = unp->pt;
        qp_skip(unp);
        qpinfo = (ti_val_t *) ti_raw_create(start, unp->pt - start);
        if (!qpinfo)
        {
            ex_set_mem(e);
            return NULL;
        }
        qpinfo->tp = TI_VAL_QP;
        return qpinfo;
    }
    case TI_KIND_C_WRAP:
    {
        qp_obj_t qp_type_id;
        ti_val_t * vthing;
        ti_wrap_t * wrap;
        uint16_t type_id;

        if (    sz != 1 ||
                !qp_is_array(qp_next(unp, NULL)) ||     /* definition */
                !qp_is_int(qp_next(unp, &qp_type_id)) ||
                !qp_is_map((sz = qp_next(unp, &qp_tmp))))
        {
            ex_set(e, EX_BAD_DATA,
                "wrap type must be written according the "
                "following syntax: {\""TI_KIND_S_WRAP"\": [type_id, {...}]");
            return NULL;
        }

        sz = sz == QP_MAP_OPEN ? -1 : sz - QP_MAP0;

        vthing = val__unp_map(unp, collection, sz, e);
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

        type_id = (uint16_t) qp_type_id.via.int64;
        wrap = ti_wrap_create((ti_thing_t *) vthing, type_id);
        if (!wrap)
            ex_set_mem(e);
        return (ti_val_t *) wrap;
    }
    }

    /* restore the unpack pointer to the first property */
    unp->pt = restore_point;
    return (ti_val_t *) ti_thing_new_from_unp(unp, collection, sz, e);
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
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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

static ti_val_t * val__from_unp(
        qp_obj_t * qp_val,
        qp_unpacker_t * unp,
        ti_collection_t * collection,
        ex_t * e)
{
    switch((qp_types_t) qp_val->tp)
    {
    case QP_RAW:
    {
        ti_raw_t * raw = ti_raw_create(qp_val->via.raw, qp_val->len);
        if (!raw)
            ex_set_mem(e);
        return (ti_val_t *) raw;
    }
    case QP_INT64:
    {
        ti_vint_t * vint = ti_vint_create(qp_val->via.int64);
        if (!vint)
            ex_set_mem(e);
        return (ti_val_t *) vint;
    }
    case QP_DOUBLE:
    {
        ti_vfloat_t * vfloat = ti_vfloat_create(qp_val->via.real);
        if (!vfloat)
            ex_set_mem(e);
        return (ti_val_t *) vfloat;
    }
    case QP_ARRAY0:
    case QP_ARRAY1:
    case QP_ARRAY2:
    case QP_ARRAY3:
    case QP_ARRAY4:
    case QP_ARRAY5:
    {
        ti_val_t * v;
        qp_obj_t qp_v;
        size_t sz = qp_val->tp - QP_ARRAY0;
        ti_varr_t * varr = ti_varr_create(sz);
        if (!varr)
        {
            ex_set_mem(e);
            return NULL;
        }

        while (sz--)
        {
            (void) qp_next(unp, &qp_v);
            v = val__from_unp(&qp_v, unp, collection, e);
            if (!v || val__push(varr, v, e))
            {
                ti_val_drop(v);
                ti_val_drop((ti_val_t *) varr);
                return NULL;  /* error `e` is set in both cases */
            }
        }
        return (ti_val_t *) varr;
    }
    case QP_MAP0:
    case QP_MAP1:
    case QP_MAP2:
    case QP_MAP3:
    case QP_MAP4:
    case QP_MAP5:
        return val__unp_map(unp, collection, (ssize_t) qp_val->tp-QP_MAP0, e);
    case QP_TRUE:
        return (ti_val_t *) ti_vbool_get(true);
    case QP_FALSE:
        return (ti_val_t *) ti_vbool_get(false);
    case QP_NULL:
        return (ti_val_t *) ti_nil_get();
    case QP_ARRAY_OPEN:
    {
        ti_val_t * v;
        qp_obj_t qp_v;
        ti_varr_t * varr = ti_varr_create(6);  /* we have at least 6 items */
        if (!varr)
        {
            ex_set_mem(e);
            return NULL;
        }

        while (!qp_is_close(qp_next(unp, &qp_v)))
        {
            v = val__from_unp(&qp_v, unp, collection, e);
            if (!v || val__push(varr, v, e))
            {
                ti_val_drop(v);
                ti_val_drop((ti_val_t *) varr);
                return NULL;  /* error `e` is set in both cases */
            }
        }
        return (ti_val_t *) varr;
    }
    case QP_MAP_OPEN:
        return val__unp_map(unp, collection, -1, e);
    case QP_END:
        ex_set(e, EX_NUM_ARGUMENTS, "missing value");
        return NULL;
    case QP_ERR:
        ex_set(e, EX_BAD_DATA, "unexpected error while unpacking value");
        return NULL;
    case QP_HOOK:
        ex_set(e, EX_BAD_DATA, "hooks are not supported");
        return NULL;
    case QP_ARRAY_CLOSE:
        ex_set(e, EX_BAD_DATA, "unexpected array close in value data");
        return NULL;
    case QP_MAP_CLOSE:
        ex_set(e, EX_BAD_DATA, "unexpected map close in value data");
        return NULL;
    }
    ex_set(e, EX_BAD_DATA, "unexpected code reached while unpacking value");
    return NULL;
}

int ti_val_init_common(void)
{
    val__sempty = (ti_val_t *) ti_raw_from_fmt("");
    val__snil = (ti_val_t *) ti_raw_from_fmt("nil");
    val__strue = (ti_val_t *) ti_raw_from_fmt("true");
    val__sfalse = (ti_val_t *) ti_raw_from_fmt("false");
    val__sblob = (ti_val_t *) ti_raw_from_fmt("<blob>");
    val__sarray = (ti_val_t *) ti_raw_from_fmt("<array>");
    val__sset = (ti_val_t *) ti_raw_from_fmt("<set>");
    val__sthing = (ti_val_t *) ti_raw_from_fmt("<thing>");
    val__sclosure = (ti_val_t *) ti_raw_from_fmt("<closure>");
    val__swrap = (ti_val_t *) ti_raw_from_fmt("<wrap>");

    if (!val__sempty || !val__snil || !val__strue || !val__sfalse ||
        !val__sblob || !val__sarray || !val__sset || !val__sthing ||
        !val__sclosure || !val__swrap)
    {
        ti_val_drop_common();
        return -1;
    }
    return 0;
}

void ti_val_drop_common(void)
{
    ti_val_drop(val__sempty);
    ti_val_drop(val__snil);
    ti_val_drop(val__strue);
    ti_val_drop(val__sfalse);
    ti_val_drop(val__sblob);
    ti_val_drop(val__sarray);
    ti_val_drop(val__sset);
    ti_val_drop(val__sthing);
    ti_val_drop(val__sclosure);
    ti_val_drop(val__swrap);
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
    case TI_VAL_QP:
    case TI_VAL_RAW:
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
ti_val_t * ti_val_from_unp(qp_unpacker_t * unp, ti_collection_t * collection)
{
    ex_t e = {0};
    qp_obj_t qp_val;
    ti_val_t * val;

    (void) qp_next(unp, &qp_val);
    val = val__from_unp(&qp_val, unp, collection, &e);
    if (e.nr)
        log_error("failed to unpack value: %s (%d) ", e.msg, e.nr);
    return val;
}

/*
 * Return NULL when failed and `e` is set to an appropriate error message
 */
ti_val_t * ti_val_from_unp_e(
        qp_unpacker_t * unp,
        ti_collection_t * collection,
        ex_t * e)
{
    qp_obj_t qp_val;
    (void) qp_next(unp, &qp_val);
    return val__from_unp(&qp_val, unp, collection, e);
}


ti_val_t * ti_val_empty_str(void)
{
    ti_incref(val__sempty);
    return val__sempty;
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

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting a scope to be of type `"TI_VAL_RAW_S"` "
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

int ti_val_convert_to_str(ti_val_t ** val)
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
        v = (ti_val_t *) ti_raw_from_strn(s, n);
        if (!v)
            return -1;
        break;
    }
    case TI_VAL_FLOAT:
    {
        size_t n;
        const char * s = strx_from_double((*(ti_vfloat_t **) val)->float_, &n);
        v = (ti_val_t *) ti_raw_from_strn(s, n);
        if (!v)
            return -1;
        break;
    }
    case TI_VAL_BOOL:
        v = (*(ti_vbool_t **) val)->bool_ ? val__strue : val__sfalse;
        ti_incref(v);
        break;
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
        if (strx_is_utf8n(
                (const char *) (*(ti_raw_t **) val)->data,
                (*(ti_raw_t **) val)->n))
            return 0;  /* do nothing */
        v = val__sblob;
        ti_incref(v);
        break;
    case TI_VAL_REGEX:
        v = (ti_val_t *) (*(ti_regex_t **) val)->pattern;
        ti_incref(v);
        break;
    case TI_VAL_THING:
        v = val__sthing;
        ti_incref(v);
        break;
    case TI_VAL_WRAP:
        v = val__swrap;
        ti_incref(v);
        break;
    case TI_VAL_ARR:
        v = val__sarray;
        ti_incref(v);
        break;
    case TI_VAL_SET:
        v = val__sset;
        ti_incref(v);
        break;
    case TI_VAL_CLOSURE:
        v = val__sclosure;
        ti_incref(v);
        break;
    case TI_VAL_ERROR:
        v = (ti_val_t *) ti_raw_from_strn(
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
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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
            val->tp == TI_VAL_RAW && ti_name_is_valid_strn(
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
    case TI_VAL_QP:
        break;
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
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

int ti_val_to_packer(ti_val_t * val, qp_packer_t ** pckr, int options)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        return qp_add_null(*pckr);
    case TI_VAL_INT:
        return qp_add_int(*pckr, ((ti_vint_t *) val)->int_);
    case TI_VAL_FLOAT:
        return qp_add_double(*pckr, ((ti_vfloat_t *) val)->float_);
    case TI_VAL_BOOL:
        return qp_add_bool(*pckr, ((ti_vbool_t *) val)->bool_);
    case TI_VAL_QP:
        return qp_add_qp(
                *pckr,
                ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_VAL_NAME:
    case TI_VAL_RAW:
        return qp_add_raw(
                *pckr,
                ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_VAL_REGEX:
        return ti_regex_to_packer((ti_regex_t *) val, pckr);
    case TI_VAL_THING:
        return ti_thing_to_packer((ti_thing_t *) val, pckr, options);
    case TI_VAL_WRAP:
    {
        ti_wrap_t * wrap = (ti_wrap_t *) val;
        if (options < 0)
        {
            if (qp_add_map(pckr) ||
                qp_add_raw(*pckr, (const uchar *) TI_KIND_S_WRAP, 1) ||
                qp_add_array(pckr) ||
                qp_add_int(*pckr, wrap->type_id))
                return -1;

            if (ti_thing_is_new(wrap->thing))
            {
                ti_thing_unmark_new(wrap->thing);
                if (ti_thing_to_packer(wrap->thing, pckr, options))
                    return -1;
            }
            else if(ti_thing_id_to_packer(wrap->thing, pckr))
                return -1;

            return qp_close_array(*pckr) || qp_close_map(*pckr);
        }
        return options > 0
            ? ti_wrap_to_packer(wrap, pckr, options)
            : ti_thing_id_to_packer(wrap->thing, pckr);
    }
    case TI_VAL_ARR:
        if (qp_add_array(pckr))
            return -1;
        for (vec_each(((ti_varr_t *) val)->vec, ti_val_t, v))
            if (ti_val_to_packer(v, pckr, options))
                return -1;
        return qp_close_array(*pckr);
    case TI_VAL_SET:
        return ti_vset_to_packer((ti_vset_t *) val, pckr, options);
    case TI_VAL_CLOSURE:
        return ti_closure_to_packer((ti_closure_t *) val, pckr);
    case TI_VAL_ERROR:
        return ti_verror_to_packer((ti_verror_t *) val, pckr);
    }

    assert(0);
    return -1;
}

int ti_val_to_file(ti_val_t * val, FILE * f)
{
    assert (val && f);

    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        return qp_fadd_type(f, QP_NULL);
    case TI_VAL_INT:
        return qp_fadd_int(f, ((ti_vint_t *) val)->int_);
    case TI_VAL_FLOAT:
        return qp_fadd_double(f, ((ti_vfloat_t *) val)->float_);
    case TI_VAL_BOOL:
        return qp_fadd_bool(f, ((ti_vbool_t *) val)->bool_);
    case TI_VAL_QP:
        return qp_fadd_qp(f, ((ti_raw_t *) val)->data, ((ti_raw_t *) val)->n);
    case TI_VAL_NAME:
    case TI_VAL_RAW:
        return qp_fadd_raw(f, ((ti_raw_t *) val)->data, ((ti_raw_t *) val)->n);
    case TI_VAL_REGEX:
        return ti_regex_to_file((ti_regex_t *) val, f);
    case TI_VAL_THING:
        return ti_thing_id_to_file((ti_thing_t *) val, f);
    case TI_VAL_WRAP:
        return ti_wrap_to_file((ti_wrap_t *) val, f);
    case TI_VAL_ARR:
    {
        vec_t * vec = ((ti_varr_t *) val)->vec;
        if (qp_fadd_type(f, vec->n > 5 ? QP_ARRAY_OPEN: QP_ARRAY0 + vec->n))
            return -1;

        for (vec_each(vec, ti_val_t, v))
            if (ti_val_to_file(v, f))
                return -1;

        return vec->n > 5 ? qp_fadd_type(f, QP_ARRAY_CLOSE) : 0;
    }
    case TI_VAL_SET:
        return ti_vset_to_file((ti_vset_t *) val, f);
    case TI_VAL_CLOSURE:
        return ti_closure_to_file((ti_closure_t *) val, f);
    case TI_VAL_ERROR:
        return ti_verror_to_file((ti_verror_t *) val, f);
    }
    assert (0);
    return -1;
}

void ti_val_may_change_pack_sz(ti_val_t * val, size_t * sz, size_t * nest)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        *sz = 16;
        *nest = 0;
        return;
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
        *sz = ((ti_raw_t *) val)->n + 16;
        *nest = 0;
        return;
    case TI_VAL_REGEX:
        *sz = ((ti_regex_t *) val)->pattern->n + 16;
        *nest = 1;
        return;
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
        *sz = 65536;
        /* do not change the nest size */
        return;
    case TI_VAL_CLOSURE:
        *sz = 4096;
        *nest = 1;
        return;
    case TI_VAL_ERROR:
        *sz = ((ti_verror_t *) val)->msg_n + 64;
        *nest = 1;
        return;
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
    case TI_VAL_QP:             return TI_VAL_INFO_S;
    case TI_VAL_NAME:
    case TI_VAL_RAW:            return TI_VAL_RAW_S;
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

