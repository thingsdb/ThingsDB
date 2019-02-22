/*
 * ti/val.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <tiinc.h>
#include <ti/arrow.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/regex.h>
#include <ti/nil.h>
#include <ti/things.h>
#include <ti/val.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/logger.h>
#include <util/strx.h>

static int val__push(ti_varr_t * arr, ti_val_t * val);
static ti_val_t * val__unp_map(qp_unpacker_t * unp, imap_t * things);
static ti_val_t * val__from_unp(
        qp_obj_t * qp_val,
        qp_unpacker_t * unp,
        imap_t * things);

static ti_val_t * val__snil;
static ti_val_t * val__strue;
static ti_val_t * val__sfalse;
static ti_val_t * val__sblob;
static ti_val_t * val__sobject;

#define VAL__BUF_SZ 128
static char val__buf[VAL__BUF_SZ];


int ti_val_init_common(void)
{
    val__snil = (ti_val_t *) ti_raw_from_strn("nil", 3);
    val__strue = (ti_val_t *) ti_raw_from_strn("true", 4);
    val__sfalse = (ti_val_t *) ti_raw_from_strn("false", 5);
    val__sblob = (ti_val_t *) ti_raw_from_strn("<blob>", 6);
    val__sobject = (ti_val_t *) ti_raw_from_strn("<object>", 8);

    if (!val__snil || !val__strue || !val__sfalse || !val__sblob ||
        !val__sobject)
    {
        ti_val_drop_common();
        return -1;
    }
    return 0;
}

void ti_val_drop_common(void)
{
    ti_val_drop(val__snil);
    ti_val_drop(val__strue);
    ti_val_drop(val__sfalse);
    ti_val_drop(val__sblob);
    ti_val_drop(val__sobject);
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
        /* no break  */
    case TI_VAL_ATTR:   /* attributes are destroyed by res */
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_QP:
    case TI_VAL_RAW:
        free(val);
        return;
    case TI_VAL_REGEX:
        ti_regex_destroy((ti_regex_t *) val);
        return;
    case TI_VAL_THING:
        ti_thing_destroy((ti_thing_t *) val);
        return;
    case TI_VAL_ARR:
        ti_varr_destroy((ti_varr_t *) val);
        return;
    case TI_VAL_ARROW:
        ti_arrow_destroy((ti_arrow_t *) val);
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
ti_val_t * ti_val_from_unp(qp_unpacker_t * unp, imap_t * things)
{
    qp_obj_t qp_val;
    qp_types_t tp = qp_next(unp, &qp_val);
    if (qp_is_close(tp))
        return NULL;
    return val__from_unp(&qp_val, unp, things);
}

int ti_val_convert_to_str(ti_val_t ** val)
{
    ti_val_t * v;
    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_ATTR:  /* attributes convert to nil and are destroyed by res */
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
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        v = val__sobject;
        ti_incref(v);
        break;
    default:
        assert(0);
        v = NULL;
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
    case TI_VAL_ATTR:  /* attributes convert to nil and are destroyed by res */
    case TI_VAL_NIL:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        ex_set(e, EX_BAD_DATA, "cannot convert type `%s` to `%s`",
                ti_val_str(*val), TI_VAL_INT_S);
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
                ex_set_alloc(e);
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
    }

    if (ti_val_make_int(val, i))
        ex_set_alloc(e);

    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}

int ti_val_convert_to_errnr(ti_val_t ** val, ex_t * e)
{
    int64_t i;
    switch((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_ATTR:  /* attributes convert to nil and are destroyed by res */
    case TI_VAL_NIL:
    case TI_VAL_QP:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        ex_set(e, EX_BAD_DATA, "cannot convert type `%s` to an `errnr`",
                ti_val_str(*val));
        return e->nr;
    case TI_VAL_INT:
        switch((*(ti_vint_t **) val)->int_)
        {
        case TI_PROTO_CLIENT_ERR_OVERFLOW:
            i = EX_OVERFLOW;
            break;
        case TI_PROTO_CLIENT_ERR_ZERO_DIV:
            i = EX_ZERO_DIV;
            break;
        case TI_PROTO_CLIENT_ERR_MAX_QUOTA:
            i = EX_MAX_QUOTA;
            break;
        case TI_PROTO_CLIENT_ERR_AUTH:
            i = EX_AUTH_ERROR;
            break;
        case TI_PROTO_CLIENT_ERR_FORBIDDEN:
            i = EX_FORBIDDEN;
            break;
        case TI_PROTO_CLIENT_ERR_INDEX:
            i = EX_INDEX_ERROR;
            break;
        case TI_PROTO_CLIENT_ERR_BAD_REQUEST:
            i = EX_BAD_DATA;
            break;
        case TI_PROTO_CLIENT_ERR_QUERY:
            i = EX_QUERY_ERROR;
            break;
        case TI_PROTO_CLIENT_ERR_NODE:
            i = EX_NODE_ERROR;
            break;
        case TI_PROTO_CLIENT_ERR_INTERNAL:
            i = EX_INTERNAL;
            break;
        default:
            ex_set(e, EX_BAD_DATA, "unknown error number `%"PRId64"`",
                    (*(ti_vint_t **) val)->int_);
            return e->nr;
        }
        break;
    case TI_VAL_RAW:
        i = (
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "OVERFLOW_ERROR",
                    strlen("OVERFLOW_ERROR")) ? EX_OVERFLOW :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "ZERO_DIV_ERROR",
                    strlen("ZERO_DIV_ERROR")) ? EX_ZERO_DIV :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "MAX_QUOTA_ERROR",
                    strlen("MAX_QUOTA_ERROR")) ? EX_MAX_QUOTA :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "AUTH_ERROR",
                    strlen("AUTH_ERROR")) ? EX_AUTH_ERROR :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "FORBIDDEN",
                    strlen("FORBIDDEN")) ? EX_FORBIDDEN :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "INDEX_ERROR",
                    strlen("INDEX_ERROR")) ? EX_INDEX_ERROR :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "BAD_REQUEST",
                    strlen("BAD_REQUEST")) ? EX_BAD_DATA :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "QUERY_ERROR",
                    strlen("QUERY_ERROR")) ? EX_QUERY_ERROR :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "NODE_ERROR",
                    strlen("NODE_ERROR")) ? EX_NODE_ERROR :
            ti_raw_equal_strn(
                    (*(ti_raw_t **) val),
                    "INTERNAL_ERROR",
                    strlen("INTERNAL_ERROR")) ? EX_INTERNAL : 0
        );
        if (!i)
        {
            ex_set(e, EX_BAD_DATA,
                    "unknown error `%.*s`",
                    (int) (*(ti_raw_t **) val)->n,
                    (const char *) (*(ti_raw_t **) val)->data);
            return e->nr;
        }
        break;
    default:
        assert (0);
        i = 0;
    }

    if (ti_val_make_int(val, i))
        ex_set_alloc(e);

    return e->nr;
}


//void ti_val_weak_copy(ti_val_t * to, ti_val_t * from)
//{
//    to->tp = from->tp;
//    to->via = from->via;
//}

//int ti_val_copy(ti_val_t * to, ti_val_t * from)
//{
//    to->tp = from->tp;
//    switch((ti_val_enum) from->tp)
//    {
//    case TI_VAL_ATTR:
//        to->via.attr = from->via.attr;  /* this is a reference */
//        return 0;
//    case TI_VAL_NIL:
//    case TI_VAL_INT:
//    case TI_VAL_FLOAT:
//    case TI_VAL_BOOL:
//        to->via = from->via;
//        return 0;
//    case TI_VAL_QP:
//    case TI_VAL_RAW:
//        to->via.raw = ti_grab(from->via.raw);
//        return 0;
//    case TI_VAL_REGEX:
//        to->via.regex = ti_grab(from->via.regex);
//        return 0;
//    case TI_VAL_TUPLE:
//    case TI_VAL_ARRAY:
//        to->via.array = vec_new(from->via.array->n);
//        if (!to->via.array)
//        {
//            to->tp = TI_VAL_NIL;
//            return -1;
//        }
//        for (vec_each(from->via.array, ti_val_t, val))
//        {
//            ti_val_t * dup = ti_val_dup(val);
//            if (!dup)
//            {
//                vec_destroy(to->via.array, (vec_destroy_cb) ti_val_drop);
//                to->tp = TI_VAL_NIL;
//                return -1;
//            }
//            VEC_push(to->via.array, dup);
//        }
//        return 0;
//    case TI_VAL_THING:
//        to->via.thing = ti_grab(from->via.thing);
//        return 0;
//    case TI_VAL_THINGS:
//        to->via.things = vec_dup(from->via.things);
//        if (!to->via.things)
//        {
//            to->tp = TI_VAL_NIL;
//            return -1;
//        }
//        for (vec_each(to->via.things, ti_thing_t, thing))
//            ti_incref(thing);
//        return 0;
//    case TI_VAL_ARROW:
//        to->via.arrow = from->via.arrow;
//        ++to->via.arrow->ref;
//        return 0;
//    }
//    log_critical("unknown type: %d", from->tp);
//    assert (0);
//    return -1;
//}

_Bool ti_val_as_bool(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        return false;
    case TI_VAL_INT:
        return !!((ti_vint_t *) val)->int_;
    case TI_VAL_FLOAT:
        return !!((ti_vfloat_t *) val)->float_;
    case TI_VAL_BOOL:
        return ((ti_vbool_t *) val)->bool_;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        return !!((ti_raw_t *) val)->n;
    case TI_VAL_REGEX:
        return true;
    case TI_VAL_ARR:
        return !!((ti_varr_t *) val)->vec->n;
    case TI_VAL_THING:
    case TI_VAL_ARROW:
        return true;
    }
    assert (0);
    return false;
}

_Bool ti_val_is_valid_name(ti_val_t * val)
{
    return val->tp == TI_VAL_RAW && ti_name_is_valid_strn(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
}

size_t ti_val_iterator_n(ti_val_t * val)
{
    switch (val->tp)
    {
    case TI_VAL_RAW:
        return ((ti_raw_t *) val)->n;
    case TI_VAL_ARR:
        return ((ti_varr_t *) val)->vec->n;
    case TI_VAL_THING:
        return ((ti_thing_t *) val)->props->n;
    }
    assert (0);
    return 0;
}

int ti_val_gen_ids(ti_val_t * val)
{
    switch (val->tp)
    {
    case TI_VAL_THING:
        /* new things 'under' an existing thing will get their own event,
         * so break */
        if (((ti_thing_t *) val)->id)
        {
            ti_thing_unmark_new((ti_thing_t *) val);
            break;
        }
        if (ti_thing_gen_id((ti_thing_t *) val))
            return -1;
        break;
    case TI_VAL_ARR:
        if (ti_varr_may_have_things((ti_varr_t *) val))
        {
            for (vec_each(((ti_varr_t *) val)->vec, ti_thing_t, thing))
            {
                if (thing->tp != TI_VAL_THING)
                    continue;

                if (thing->id)
                {
                    ti_thing_unmark_new(thing);
                    continue;
                }
                if (ti_thing_gen_id(thing))
                    return -1;
            }
        }
    }
    return 0;
}

int ti_val_to_packer(ti_val_t * val, qp_packer_t ** pckr, int flags, int fetch)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:
//        return flags & TI_VAL_PACK_NEW
//            ? qp_add_null(*pckr)
//            : ti_val_to_packer(
//                &((ti_vattr_t *) val)->prop->val,
//                pckr,
//                flags,
//                fetch
//            );
        assert (0);
        return 0;
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
    case TI_VAL_RAW:
        return qp_add_raw(
                *pckr,
                ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_VAL_REGEX:
        return ti_regex_to_packer((ti_regex_t *) val, pckr);
    case TI_VAL_THING:
        return flags & TI_VAL_PACK_NEW
            ? (((ti_thing_t *) val)->flags & TI_THING_FLAG_NEW
                ? ti_thing_to_packer((ti_thing_t *) val, pckr, flags, fetch)
                : ti_thing_id_to_packer((ti_thing_t *) val, pckr))
            : (fetch-- > 0
                ? ti_thing_to_packer((ti_thing_t *) val, pckr, flags, fetch)
                : ti_thing_id_to_packer((ti_thing_t *) val, pckr));
    case TI_VAL_ARR:
        if (qp_add_array(pckr))
            return -1;
        fetch--;
        for (vec_each(((ti_varr_t *) val)->vec, ti_val_t, v))
            if (ti_val_to_packer(v, pckr, flags, fetch))
                return -1;
        return qp_close_array(*pckr);
    case TI_VAL_ARROW:
        return ti_arrow_to_packer((ti_arrow_t *) val, pckr);
    }

    assert(0);
    return -1;
}

int ti_val_to_file(ti_val_t * val, FILE * f)
{
    assert (val && f);

    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_QP:
        assert (0);
        return -1;
    case TI_VAL_NIL:
        return qp_fadd_type(f, QP_NULL);
    case TI_VAL_INT:
        return qp_fadd_int(f, ((ti_vint_t *) val)->int_);
    case TI_VAL_FLOAT:
        return qp_fadd_double(f, ((ti_vfloat_t *) val)->float_);
    case TI_VAL_BOOL:
        return qp_fadd_type(f, ((ti_vbool_t *) val)->bool_ ? QP_TRUE:QP_FALSE);
    case TI_VAL_RAW:
        return qp_fadd_raw(f, ((ti_raw_t *) val)->data, ((ti_raw_t *) val)->n);
    case TI_VAL_REGEX:
        return ti_regex_to_file((ti_regex_t *) val, f);
    case TI_VAL_THING:
        return (
                qp_fadd_type(f, QP_MAP1) ||
                qp_fadd_raw(f, (const uchar *) "#", 1) ||
                qp_fadd_int(f, ((ti_thing_t *) val)->id)
        );
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
    case TI_VAL_ARROW:
        return ti_arrow_to_file((ti_arrow_t *) val, f);
    }
    assert (0);
    return -1;
}

const char * ti_val_str(ti_val_t * val)
{
    switch (val->tp)
    {
    case TI_VAL_ATTR:               return TI_VAL_ATTR_S;
    case TI_VAL_NIL:                return TI_VAL_NIL_S;
    case TI_VAL_INT:                return TI_VAL_INT_S;
    case TI_VAL_FLOAT:              return TI_VAL_FLOAT_S;
    case TI_VAL_BOOL:               return TI_VAL_BOOL_S;
    case TI_VAL_QP:                 return TI_VAL_QP_S;
    case TI_VAL_RAW:                return TI_VAL_RAW_S;
    case TI_VAL_REGEX:              return TI_VAL_REGEX_S;
    case TI_VAL_THING:              return TI_VAL_THING_S;
    case TI_VAL_ARR:                return ti_varr_is_list((ti_varr_t *) val)
                                        ? TI_VAL_ARR_LIST_S
                                        : TI_VAL_ARR_TUPLE_S;
    case TI_VAL_ARROW:              return TI_VAL_ARROW_S;
    }
    assert (0);
    return "unknown";
}


/* checks PROP, QP, ARROW and ARRAY/TUPLE */
int ti_val_make_assignable(ti_val_t * val, ex_t * e)
{
    switch (val->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_QP:
        ex_set(e, EX_BAD_DATA, "type `%s` cannot be assigned",
                ti_val_str(val));
        break;
    case TI_VAL_ARROW:
        if (ti_arrow_wse((ti_arrow_t * ) val))
        {
            ex_set(e, EX_BAD_DATA,
                    "an arrow function with side effects cannot be assigned");
        }
        else if (ti_arrow_unbound((ti_arrow_t * ) val))
            ex_set_alloc(e);
        break;
    }
    return e->nr;
}

static ti_val_t * val__unp_map(qp_unpacker_t * unp, imap_t * things)
{
    qp_obj_t qp_kind, qp_tmp;
    ssize_t sz = qp_next(unp, NULL);

    assert (qp_is_map(sz));

    sz = sz == QP_MAP_OPEN ? -1 : sz - QP_MAP0;

    if (!sz || !qp_is_raw(qp_next(unp, &qp_kind)) || qp_kind.len != 1)
        return NULL;

    switch ((ti_val_kind) *qp_kind.via.raw)
    {
    case TI_VAL_KIND_THING:
        return qp_is_int(qp_next(unp, &qp_tmp))
                ? (ti_val_t *) ti_things_thing_from_unp(
                        things,
                        (uint64_t) qp_tmp.via.int64,
                        unp,
                        sz)
                : NULL;
    case TI_VAL_KIND_ARROW:
        if (sz != 1 || !qp_is_raw(qp_next(unp, &qp_tmp)))
            return NULL;
        return (ti_val_t *) ti_arrow_from_strn(
                (char *) qp_tmp.via.raw,
                qp_tmp.len);
    case TI_VAL_KIND_REGEX:
    {
        ex_t e = { .nr=0 };
        ti_regex_t * regex;

        if (sz != 1 || !qp_is_raw(qp_next(unp, &qp_tmp)))
            return NULL;
        regex = ti_regex_from_strn(
                (const char *) qp_tmp.via.raw,
                qp_tmp.len,
                &e);
        if (!regex)
            log_error(e.msg);
        return (ti_val_t *) regex;
    }
    }
    assert (0);
    return NULL;
}

/*
 * Does not increment the `val` reference counter.
 */
static int val__push(ti_varr_t * varr, ti_val_t * val)
{
    assert (ti_varr_is_list(varr));

    if (ti_val_is_list(val))
        ((ti_varr_t *) val)->flags |= TI_ARR_FLAG_TUPLE;

    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:
        varr->flags |= TI_ARR_FLAG_THINGS;
        break;
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        break;
    case TI_VAL_ATTR:
    case TI_VAL_QP:
        return -1;
    }

    return vec_push(&varr->vec, val);
}

static ti_val_t * val__from_unp(
        qp_obj_t * qp_val,
        qp_unpacker_t * unp,
        imap_t * things)
{
    switch(qp_val->tp)
    {
    case QP_RAW:
        return (ti_val_t *) ti_raw_create(qp_val->via.raw, qp_val->len);
    case QP_INT64:
        return (ti_val_t *) ti_vint_create(qp_val->via.int64);
    case QP_DOUBLE:
        return (ti_val_t *) ti_vfloat_create(qp_val->via.real);
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
            return NULL;

        while (sz--)
        {
            (void) qp_next(unp, &qp_v);
            v = val__from_unp(&qp_v, unp, things);
            if (!v || val__push(varr, v))
            {
                ti_val_drop(v);
                ti_val_drop((ti_val_t *) varr);
                return NULL;
            }
        }
        return (ti_val_t *) varr;
    }
    case QP_MAP1:
    case QP_MAP2:
    case QP_MAP3:
    case QP_MAP4:
    case QP_MAP5:
        --unp->pt;  /* reset to map */
        return val__unp_map(unp, things);
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
            return NULL;

        while (!qp_is_close(qp_next(unp, &qp_v)))
        {
            v = val__from_unp(&qp_v, unp, things);
            if (!v || val__push(varr, v))
            {
                ti_val_drop(v);
                ti_val_drop((ti_val_t *) varr);
                return NULL;
            }
        }
        return (ti_val_t *) varr;
    }
    case QP_MAP_OPEN:
        --unp->pt;  /* reset to map */
        return val__unp_map(unp, things);
    default:
        assert (0);
        return NULL;
    }

    return NULL;
}


