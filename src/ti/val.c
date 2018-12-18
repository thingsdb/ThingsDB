/*
 * ti/val.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti/val.h>
#include <ti/arrow.h>
#include <ti/prop.h>
#include <ti/things.h>
#include <ti.h>
#include <util/logger.h>

static int val__unp_map(ti_val_t * dest, qp_unpacker_t * unp, imap_t * things);
static int val__push(ti_val_t * arr, ti_val_t * val);
static int val__from_unp(
        ti_val_t * dest,
        qp_obj_t * qp_val,
        qp_unpacker_t * unp,
        imap_t * things);

ti_val_t * ti_val_create(ti_val_enum tp, void * v)
{
    ti_val_t * val = malloc(sizeof(ti_val_t));
    if (!val)
        return NULL;
    val->flags = 0;
    if (ti_val_set(val, tp, v))
    {
        ti_val_destroy(val);
        return NULL;
    }
    return val;
}

ti_val_t * ti_val_weak_create(ti_val_enum tp, void * v)
{
    ti_val_t * val = malloc(sizeof(ti_val_t));
    if (!val)
        return NULL;
    val->flags = 0;
    ti_val_weak_set(val, tp, v);
    return val;
}

ti_val_t * ti_val_dup(ti_val_t * val)
{
    ti_val_t * dup = malloc(sizeof(ti_val_t));
    if (!dup)
        return NULL;
    if (ti_val_copy(dup, val))
    {
        free(dup);
        return NULL;
    }
    return dup;
}

ti_val_t * ti_val_weak_dup(ti_val_t * val)
{
    ti_val_t * dup = malloc(sizeof(ti_val_t));
    if (!dup)
        return NULL;
    memcpy(dup, val, sizeof(ti_val_t));
    return dup;
}

/*
 * Return 0 on success, <0 if unpacking has failed or >0 if
 */
int ti_val_from_unp(ti_val_t * dest, qp_unpacker_t * unp, imap_t * things)
{
    qp_obj_t qp_val;
    qp_types_t tp = qp_next(unp, &qp_val);
    return qp_is_close(tp)
            ? (int) tp
            : val__from_unp(dest, &qp_val, unp, things);
}

void ti_val_destroy(ti_val_t * val)
{
    if (!val)
        return;
    ti_val_clear(val);
    free(val);
}

void ti_val_weak_set(ti_val_t * val, ti_val_enum tp, void * v)
{
    val->tp = tp;
    val->flags = 0;
    switch((ti_val_enum) tp)
    {
    case TI_VAL_ATTR:
        val->via.attr = v;
        return;
    case TI_VAL_NIL:
        val->via.nil = NULL;
        return;
    case TI_VAL_INT:
        {
            int64_t * p = v;
            val->via.int_ = *p;
        }
        return;
    case TI_VAL_FLOAT:
        {
            double * p = v;
            val->via.float_ = *p;
        }
        return;
    case TI_VAL_BOOL:
        {
            _Bool * p = v;
            val->via.bool_ = !!*p;
        }
        return;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        val->via.raw = v;
        return;
    case TI_VAL_REGEX:
        val->via.regex = v;
        return;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        val->via.array = v;
        return;
    case TI_VAL_THING:
        val->via.thing = v;
        return;
    case TI_VAL_THINGS:
        val->via.things = v;
        return;
    case TI_VAL_ARROW:
        val->via.arrow = v;
        return;
    }
    log_critical("unknown type: %d", tp);
    assert (0);
}

int ti_val_set(ti_val_t * val, ti_val_enum tp, void * v)
{
    val->tp = tp;
    val->flags = 0;
    switch((ti_val_enum) tp)
    {
    case TI_VAL_ATTR:
        val->via.attr = v;
        return 0;
    case TI_VAL_NIL:
        val->via.nil = NULL;
        return 0;
    case TI_VAL_INT:
        {
            int64_t * p = v;
            val->via.int_ = *p;
        }
        return 0;
    case TI_VAL_FLOAT:
        {
            double * p = v;
            val->via.float_ = *p;
        }
        return 0;
    case TI_VAL_BOOL:
        {
            _Bool * p = v;
            val->via.bool_ = !!*p;
        }
        return 0;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        val->via.raw = ti_grab((ti_raw_t *) v);
        return 0;
    case TI_VAL_REGEX:
        val->via.regex = ti_grab((ti_regex_t *) v);
        return 0;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        val->via.array = vec_dup((vec_t *) v);
        if (!val->via.array)
        {
            val->tp = TI_VAL_NIL;
            return -1;
        }
        return 0;
    case TI_VAL_THING:
        val->via.thing = ti_grab((ti_thing_t *) v);
        return 0;
    case TI_VAL_THINGS:
        val->via.things = vec_dup((vec_t *) v);
        if (!val->via.things)
        {
            val->tp = TI_VAL_NIL;
            return -1;
        }
        for (vec_each(val->via.things, ti_thing_t, thing))
            ti_incref(thing);
        return 0;
    case TI_VAL_ARROW:
        val->via.arrow = v;
        ++val->via.arrow->ref;
        return 0;
    }
    log_critical("unknown type: %d", tp);
    assert (0);
    return -1;
}

void ti_val_weak_copy(ti_val_t * to, ti_val_t * from)
{
    to->tp = from->tp;
    to->flags = 0;
    to->via = from->via;
}

int ti_val_copy(ti_val_t * to, ti_val_t * from)
{
    to->tp = from->tp;
    to->flags = 0;
    switch((ti_val_enum) from->tp)
    {
    case TI_VAL_ATTR:
        to->via.attr = from->via.attr;  /* this is a reference */
        return 0;
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        to->via = from->via;
        return 0;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        to->via.raw = ti_grab(from->via.raw);
        return 0;
    case TI_VAL_REGEX:
        to->via.regex = ti_grab(from->via.regex);
        return 0;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        to->via.array = vec_new(from->via.array->n);
        if (!to->via.array)
        {
            to->tp = TI_VAL_NIL;
            return -1;
        }
        for (vec_each(from->via.array, ti_val_t, val))
        {
            ti_val_t * dup = ti_val_dup(val);
            if (!dup)
            {
                vec_destroy(to->via.array, (vec_destroy_cb) ti_val_destroy);
                to->tp = TI_VAL_NIL;
                return -1;
            }
            VEC_push(to->via.array, dup);
        }
        return 0;
    case TI_VAL_THING:
        to->via.thing = ti_grab(from->via.thing);
        return 0;
    case TI_VAL_THINGS:
        to->via.things = vec_dup(from->via.things);
        if (!to->via.things)
        {
            to->tp = TI_VAL_NIL;
            return -1;
        }
        for (vec_each(to->via.things, ti_thing_t, thing))
            ti_incref(thing);
        return 0;
    case TI_VAL_ARROW:
        to->via.arrow = from->via.arrow;
        ++to->via.arrow->ref;
        return 0;
    }
    log_critical("unknown type: %d", from->tp);
    assert (0);
    return -1;
}

void ti_val_set_arrow(ti_val_t * val, cleri_node_t * arrow_nd)
{
    val->tp = TI_VAL_ARROW;
    val->flags = 0;
    val->via.arrow = arrow_nd;
    ++arrow_nd->ref;
}

void ti_val_set_bool(ti_val_t * val, _Bool bool_)
{
    val->tp = TI_VAL_BOOL;
    val->flags = 0;
    val->via.bool_ = !!bool_;
}

void ti_val_set_nil(ti_val_t * val)
{
    val->tp = TI_VAL_NIL;
    val->flags = 0;
    val->via.nil = NULL;
}

void ti_val_set_int(ti_val_t * val, int64_t i)
{
    val->tp = TI_VAL_INT;
    val->flags = 0;
    val->via.int_ = i;
}

void ti_val_set_float(ti_val_t * val, double d)
{
    val->tp = TI_VAL_FLOAT;
    val->flags = 0;
    val->via.float_ = d;
}

void ti_val_set_thing(ti_val_t * val, ti_thing_t * thing)
{
    val->tp = TI_VAL_THING;
    val->flags = 0;
    val->via.thing = ti_grab(thing);
}

_Bool ti_val_as_bool(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        return false;
    case TI_VAL_INT:
        return !!val->via.int_;
    case TI_VAL_FLOAT:
        return !!val->via.float_;
    case TI_VAL_BOOL:
        return val->via.bool_;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        return !!val->via.raw->n;
    case TI_VAL_REGEX:
        return true;
    case TI_VAL_ARRAY:
    case TI_VAL_TUPLE:
    case TI_VAL_THINGS:
        return !!val->via.arr->n;
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
                (const char *) val->via.raw->data,
                val->via.raw->n);
}

size_t ti_val_iterator_n(ti_val_t * val)
{
    switch (val->tp)
    {
    case TI_VAL_RAW:
        return val->via.raw->n;
    case TI_VAL_ARRAY:
    case TI_VAL_TUPLE:
    case TI_VAL_THINGS:
        return val->via.arr->n;
    case TI_VAL_THING:
        return val->via.thing->props->n;
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
        if (val->via.thing->id)
        {
            ti_thing_unmark_new(val->via.thing);
            break;
        }
        if (ti_thing_gen_id(val->via.thing))
            return -1;
        break;
    case TI_VAL_THINGS:
        for (vec_each(val->via.things, ti_thing_t, thing))
        {
            if (thing->id)
            {
                ti_thing_unmark_new(thing);
                continue;
            }
            if (ti_thing_gen_id(thing))
                return -1;
        }
    }
    return 0;
}

/*
 * Clear value and sets nil
 */
void ti_val_clear(ti_val_t * val)
{
    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:  /* attributes are destroyed by res */
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        ti_raw_drop(val->via.raw);
        break;
    case TI_VAL_REGEX:
        ti_regex_drop(val->via.regex);
        break;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        vec_destroy(val->via.array, (vec_destroy_cb) ti_val_destroy);
        break;
    case TI_VAL_THING:
        ti_thing_drop(val->via.thing);
        break;
    case TI_VAL_THINGS:
        vec_destroy(val->via.things, (vec_destroy_cb) ti_thing_drop);
        break;
    case TI_VAL_ARROW:
        if (!val->via.arrow || --val->via.arrow->ref)
            break;

        if (val->via.arrow->str == val->via.arrow->data)
            free(val->via.arrow->data);
        /*
         * We need to restore one reference since cleri__node_free will remove
         * one
         */
        ++val->via.arrow->ref;
        cleri__node_free(val->via.arrow);
        break;
    }
    val->flags = 0;
    val->via.nil = NULL;
    val->tp = TI_VAL_NIL;
}

int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer, int flags)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:
        /* this is for value packing when the parent is not a `thing` */
        assert (~flags & TI_VAL_PACK_THING);

        return flags & TI_VAL_PACK_NEW
            ? qp_add_null(*packer)
            : ti_val_to_packer(
                &((ti_prop_t *) val->via.attr)->val,
                packer,
                flags
            );
    case TI_VAL_NIL:
        return qp_add_null(*packer);
    case TI_VAL_INT:
        return qp_add_int64(*packer, val->via.int_);
    case TI_VAL_FLOAT:
        return qp_add_double(*packer, val->via.float_);
    case TI_VAL_BOOL:
        return val->via.bool_ ?
                qp_add_true(*packer) : qp_add_false(*packer);
    case TI_VAL_QP:
        return qp_add_qp(*packer, val->via.raw->data, val->via.raw->n);
    case TI_VAL_RAW:
        return qp_add_raw(*packer, val->via.raw->data, val->via.raw->n);
    case TI_VAL_REGEX:
        return ti_regex_to_packer(val->via.regex, packer);
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        if (qp_add_array(packer))
            return -1;
        for (vec_each(val->via.array, ti_val_t, v))
        {
            if (ti_val_to_packer(v, packer, flags))
                return -1;
        }
        return qp_close_array(*packer);
    case TI_VAL_THING:
        return flags & TI_VAL_PACK_NEW
                ? (val->via.thing->flags & TI_THING_FLAG_NEW
                    ? ti_thing_to_packer(val->via.thing, packer, flags)
                    : ti_thing_id_to_packer(val->via.thing, packer))
                : (val->flags & TI_VAL_FLAG_FETCH
                    ? ti_thing_to_packer(val->via.thing, packer, flags)
                    : ti_thing_id_to_packer(val->via.thing, packer));
    case TI_VAL_THINGS:
        if (qp_add_array(packer))
            return -1;
        for (vec_each(val->via.things, ti_thing_t, thing))
        {
            if (    flags == TI_VAL_PACK_NEW &&
                    (val->via.thing->flags & TI_THING_FLAG_NEW))
            {
                if (ti_thing_to_packer(val->via.thing, packer, flags))
                    return -1;
                continue;
            }

            if (ti_thing_id_to_packer(thing, packer))
                return -1;
        }
        return qp_close_array(*packer);
    case TI_VAL_ARROW:
        return ti_arrow_to_packer(val->via.arrow, packer);
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
        return qp_fadd_int64(f, val->via.int_);
    case TI_VAL_FLOAT:
        return qp_fadd_double(f, val->via.float_);
    case TI_VAL_BOOL:
        return qp_fadd_type(f, val->via.bool_ ? QP_TRUE : QP_FALSE);
    case TI_VAL_RAW:
        return qp_fadd_raw(f, val->via.raw->data, val->via.raw->n);
    case TI_VAL_REGEX:
        return ti_regex_to_file(val->via.regex, f);
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        if (qp_fadd_type(f, QP_ARRAY_OPEN))
            return -1;
        for (vec_each(val->via.things, ti_val_t, v))
        {
            if (ti_val_to_file(v, f))
                return -1;
        }
        return qp_fadd_type(f, QP_ARRAY_CLOSE);
    case TI_VAL_THING:
        return qp_fadd_int64(f, val->via.thing->id);
    case TI_VAL_THINGS:
        if (qp_fadd_type(f, QP_ARRAY_OPEN))
            return -1;
        for (vec_each(val->via.things, ti_thing_t, thing))
        {
            if (qp_fadd_int64(f, thing->id))
                return -1;
        }
        return qp_fadd_type(f, QP_ARRAY_CLOSE);
    case TI_VAL_ARROW:
        return ti_arrow_to_file(val->via.arrow, f);
    }
    assert (0);
    return -1;
}

const char * ti_val_tp_str(ti_val_enum tp)
{
    switch (tp)
    {
    case TI_VAL_ATTR:               return "attribute";
    case TI_VAL_NIL:                return "nil";
    case TI_VAL_INT:                return "int";
    case TI_VAL_FLOAT:              return "float";
    case TI_VAL_BOOL:               return "bool";
    case TI_VAL_QP:                 return "qpack";
    case TI_VAL_RAW:                return "raw";
    case TI_VAL_REGEX:              return "regex";
    case TI_VAL_TUPLE:              return "tuple";
    case TI_VAL_ARRAY:
    case TI_VAL_THINGS:             return "array";
    case TI_VAL_THING:              return "thing";
    case TI_VAL_ARROW:              return "arrow-function";
    }
    assert (0);
    return "unknown";
}

_Bool ti_val_startswith(ti_val_t * a, ti_val_t * b)
{
    uchar * au, * bu;

    if (a->tp != TI_VAL_RAW ||
        b->tp != TI_VAL_RAW ||
        a->via.raw->n < b->via.raw->n)
        return false;

    au = a->via.raw->data;
    bu = b->via.raw->data;
    for (size_t n = b->via.raw->n; n; --n, ++au, ++bu)
        if (*au != *bu)
            return false;
    return true;
}

_Bool ti_val_endswith(ti_val_t * a, ti_val_t * b)
{
    uchar * au, * bu;

    if (a->tp != TI_VAL_RAW ||
        b->tp != TI_VAL_RAW ||
        a->via.raw->n < b->via.raw->n)
        return false;

    au = a->via.raw->data + a->via.raw->n;
    bu = b->via.raw->data + b->via.raw->n;

    for (size_t n = b->via.raw->n; n; --n)
    {
        if (*--au != *--bu)
            return false;
    }

    return true;
}

/* The destination array should have enough space to hold the new value.
 * Note that 'val' will be moved so should not be used after calling thing
 * function.
 */
int ti_val_move_to_arr(ti_val_t * to_arr, ti_val_t * val, ex_t * e)
{
    assert (ti_val_is_mutable_arr(to_arr));
    assert (vec_space(to_arr->via.arr));

    if (val->tp == TI_VAL_THINGS)
    {
        if (val->via.things->n)
        {
            ex_set(e, EX_BAD_DATA,
                    "type `%s` cannot be nested into into another array",
                    ti_val_str(val));
            return e->nr;
        }
        val->tp = TI_VAL_TUPLE;
    }

    if (val->tp == TI_VAL_THING)
    {
        /* TODO: I think we can convert back, not sure why I first thought
         * this was not possible. Maybe because of nesting? but that is
         * solved because nested are tuple and therefore not mutable */
        if (to_arr->tp == TI_VAL_ARRAY  && !to_arr->via.array->n)
            to_arr->tp = TI_VAL_THINGS;

        if (to_arr->tp == TI_VAL_ARRAY)
        {
            ex_set(e, EX_BAD_DATA,
                "type `%s` cannot be added into an array with other types",
                ti_val_str(val));
            return e->nr;
        }

        VEC_push(to_arr->via.things, val->via.thing);
        ti_val_weak_destroy(val);
        return 0;
    }

    if (to_arr->tp == TI_VAL_THINGS  && !to_arr->via.things->n)
        to_arr->tp = TI_VAL_ARRAY;

    if (to_arr->tp == TI_VAL_THINGS)
    {
        ex_set(e, EX_BAD_DATA,
            "type `%s` cannot be added into an array with `things`",
            ti_val_str(val));
        return e->nr;
    }

    if (ti_val_check_assignable(val, true, e))
        return e->nr;

    VEC_push(to_arr->via.array, val);
    return 0;
}

/* checks PROP, QP, ARROW and ARRAY/TUPLE */
int ti_val_check_assignable(ti_val_t * val, _Bool to_array, ex_t * e)
{
    switch (val->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_QP:
        ex_set(e, EX_BAD_DATA, "type `%s` cannot be assigned",
                ti_val_tp_str(val->tp));
        break;
    case TI_VAL_ARROW:
        if (ti_arrow_wse(val->via.arrow))
            ex_set(e, EX_BAD_DATA,
                    "an arrow function with side effects cannot be assigned");
        break;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        val->tp = to_array ? TI_VAL_TUPLE : TI_VAL_ARRAY;
        break;
    }
    return e->nr;
}

static int val__unp_map(ti_val_t * dest, qp_unpacker_t * unp, imap_t * things)
{
    qp_obj_t qp_kind, qp_tmp;
    ssize_t sz = qp_next(unp, NULL);

    assert (qp_is_map(sz));

    sz = sz == QP_MAP_OPEN ? -1 : sz - QP_MAP0;

    if (!sz || !qp_is_raw(qp_next(unp, &qp_kind)) || qp_kind.len != 1)
        goto fail;

    switch ((ti_val_kind) *qp_kind.via.raw)
    {
    case TI_VAL_KIND_THING:
    {
        uint64_t thing_id;
        if (!qp_is_int(qp_next(unp, &qp_tmp)))
            goto fail;
        thing_id = (uint64_t) qp_tmp.via.int64;
        dest->tp = TI_VAL_THING;
        dest->via.thing = ti_things_thing_from_unp(things, thing_id, unp, sz);
        if (!dest->via.thing)
            return -1;
        break;
    }
    case TI_VAL_KIND_ARROW:
        if (sz != 1 || !qp_is_raw(qp_next(unp, &qp_tmp)))
            goto fail;
        dest->tp = TI_VAL_ARROW;
        dest->via.arrow = ti_arrow_from_strn(
                (char *) qp_tmp.via.raw,
                qp_tmp.len);
        if (!dest->via.arrow)
            return -1;
        break;
    case TI_VAL_KIND_REGEX:
    {
        ex_t * e = ex_use();

        if (sz != 1 || !qp_is_raw(qp_next(unp, &qp_tmp)))
            goto fail;
        dest->tp = TI_VAL_REGEX;
        dest->via.regex = ti_regex_from_strn(
                (const char *) qp_tmp.via.raw,
                qp_tmp.len,
                e);
        if (!dest->via.regex)
        {
            log_error(e->msg);
            return -1;
        }
        break;
    }
    }

    return 0;

fail:
    dest->via.nil = NULL;
    dest->tp = TI_VAL_NIL;
    return -1;
}

static int val__push(ti_val_t * arr, ti_val_t * val)
{
    if (    val->tp == TI_VAL_ARRAY || (
            val->tp == TI_VAL_THINGS && !val->via.things->n))
        val->tp = TI_VAL_TUPLE;

    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_THING:
        return -(
            arr->tp != TI_VAL_THINGS ||
            vec_push(&arr->via.things, val->via.thing)
        );
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_TUPLE:
    case TI_VAL_ARROW:
    {
        ti_val_t * v;
        if (arr->tp != TI_VAL_ARRAY)
        {
            assert (arr->tp == TI_VAL_THINGS);
            if (arr->via.things->n)
                return -1;
            arr->tp = TI_VAL_ARRAY;
        }
        v = ti_val_weak_dup(val);
        return v ? vec_push(&arr->via.array, v) : -1;
    }
    case TI_VAL_ATTR:
    case TI_VAL_QP:
    case TI_VAL_ARRAY:
    case TI_VAL_THINGS:
        return -1;
    }
    return -1;
}

static int val__from_unp(
        ti_val_t * dest,
        qp_obj_t * qp_val,
        qp_unpacker_t * unp,
        imap_t * things)
{
    switch(qp_val->tp)
    {
    case QP_RAW:
        dest->tp = TI_VAL_RAW;
        dest->via.raw = ti_raw_create(qp_val->via.raw, qp_val->len);
        if (!dest->via.raw)
            goto fail;
        break;
    case QP_INT64:
        dest->tp = TI_VAL_INT;
        dest->via.int_ = qp_val->via.int64;
        break;
    case QP_DOUBLE:
        dest->tp = TI_VAL_FLOAT;
        dest->via.float_ = qp_val->via.real;
        break;
    case QP_ARRAY0:
    case QP_ARRAY1:
    case QP_ARRAY2:
    case QP_ARRAY3:
    case QP_ARRAY4:
    case QP_ARRAY5:
    {
        qp_obj_t qp_v;
        ti_val_t v;
        size_t sz = qp_val->tp - QP_ARRAY0;
        dest->tp = TI_VAL_THINGS;
        dest->via.arr = vec_new(sz);
        if (!dest->via.arr)
            goto fail;

        while (sz--)
        {
            (void) qp_next(unp, &qp_v);

            if (val__from_unp(&v, &qp_v, unp, things) || val__push(dest, &v))
            {
                ti_val_clear(&v);
                goto fail;
            }
        }

        break;
    }
    case QP_MAP1:
    case QP_MAP2:
    case QP_MAP3:
    case QP_MAP4:
    case QP_MAP5:
        --unp->pt;  /* reset to map */
        if (val__unp_map(dest, unp, things))
            goto fail;
        break;
    case QP_TRUE:
    case QP_FALSE:
        dest->tp = TI_VAL_BOOL;
        dest->via.bool_ = qp_val->tp == QP_TRUE;
        break;
    case QP_NULL:
        dest->tp = TI_VAL_NIL;
        dest->via.nil = NULL;
        break;
    case QP_ARRAY_OPEN:
    {
        ti_val_t v;
        qp_obj_t qp_v;
        dest->tp = TI_VAL_THINGS;
        dest->via.arr = vec_new(6);  /* most likely we have at least 6 items */
        if (!dest->via.arr)
            goto fail;

        while (1)
        {
            if (qp_is_close(qp_next(unp, &qp_v)))
                break;

            if (val__from_unp(&v, &qp_v, unp, things) || val__push(dest, &v))
            {
                ti_val_clear(&v);
                goto fail;
            }
        }
        break;
    }
    case QP_MAP_OPEN:
        --unp->pt;  /* reset to map */
        if (val__unp_map(dest, unp, things))
            goto fail;
        break;
    default:
        goto fail;
    }

    return 0;

fail:
    ti_val_clear(dest);
    return -1;
}
