/*
 * val.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti/val.h>
#include <ti/arrow.h>
#include <ti/prop.h>
#include <ti.h>
#include <util/logger.h>

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
    case TI_VAL_UNDEFINED:
        val->via.undefined = NULL;
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
    case TI_VAL_NAME:
        val->via.name = v;
        return;
    case TI_VAL_RAW:
        val->via.raw = v;
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
    case TI_VAL_UNDEFINED:
        val->via.undefined = NULL;
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
    case TI_VAL_NAME:
        val->via.name = ti_grab((ti_name_t *) v);
        return 0;
    case TI_VAL_RAW:
        val->via.raw = ti_raw_dup((ti_raw_t *) v);
        if (!val->via.raw)
        {
            val->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        return 0;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        val->via.array = vec_dup((vec_t *) v);
        if (!val->via.array)
        {
            val->tp = TI_VAL_UNDEFINED;
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
            val->tp = TI_VAL_UNDEFINED;
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
    case TI_VAL_UNDEFINED:
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        to->via = from->via;
        return 0;
    case TI_VAL_NAME:
        to->via.name = ti_grab(from->via.name);
        return 0;
    case TI_VAL_RAW:
        to->via.raw = ti_raw_dup(from->via.raw);
        if (!to->via.raw)
        {
            to->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        return 0;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        to->via.array = vec_new(from->via.array->n);
        if (!to->via.array)
        {
            to->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        for (vec_each(from->via.array, ti_val_t, val))
        {
            ti_val_t * dup = ti_val_dup(val);
            if (!dup)
            {
                vec_destroy(to->via.array, (vec_destroy_cb) ti_val_destroy);
                to->tp = TI_VAL_UNDEFINED;
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
            to->tp = TI_VAL_UNDEFINED;
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

void ti_val_set_undefined(ti_val_t * val)
{
    val->tp = TI_VAL_UNDEFINED;
    val->flags = 0;
    val->via.undefined = NULL;
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

_Bool ti_val_as_bool(ti_val_t * val)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_UNDEFINED:
    case TI_VAL_NIL:
        return false;
    case TI_VAL_INT:
        return !!val->via.int_;
    case TI_VAL_FLOAT:
        return !!val->via.float_;
    case TI_VAL_BOOL:
        return val->via.bool_;
    case TI_VAL_NAME:
        return !!val->via.name->n;
    case TI_VAL_RAW:
        return !!val->via.raw->n;
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
    return val->tp == TI_VAL_RAW
        ? ti_name_is_valid_strn(
                (const char *) val->via.raw->data,
                val->via.raw->n)
        : val->tp == TI_VAL_NAME;
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
 * Clear value
 */
void ti_val_clear(ti_val_t * val)
{
    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:  /* attributes are destroyed by res */
    case TI_VAL_UNDEFINED:
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        break;
    case TI_VAL_NAME:
        ti_name_drop(val->via.name);
        break;
    case TI_VAL_RAW:
        ti_raw_free(val->via.raw);
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
        if (val->via.arrow->str == val->via.arrow->data)
            free(val->via.arrow->data);
        cleri__node_free(val->via.arrow);
        break;
    }
    val->flags = 0;
    val->tp = TI_VAL_UNDEFINED;
}

int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer, int pack)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_ATTR:
        if (pack == TI_VAL_PACK_NEW)
            return qp_add_null(*packer);
        ti_prop_t * attr = (ti_prop_t *) val->via.attr;
        assert (attr->val.tp != TI_VAL_UNDEFINED);
        return ti_val_to_packer(&attr->val, packer, pack);
    case TI_VAL_UNDEFINED:
        assert (0);
        return 0;
    case TI_VAL_NIL:
        return qp_add_null(*packer);
    case TI_VAL_INT:
        return qp_add_int64(*packer, val->via.int_);
    case TI_VAL_FLOAT:
        return qp_add_double(*packer, val->via.float_);
    case TI_VAL_BOOL:
        return val->via.bool_ ?
                qp_add_true(*packer) : qp_add_false(*packer);
    case TI_VAL_NAME:
        return qp_add_raw(
                *packer,
                (uchar *) val->via.name->str,
                val->via.name->n);
    case TI_VAL_RAW:
        return qp_add_raw(*packer, val->via.raw->data, val->via.raw->n);
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        if (qp_add_array(packer))
            return -1;
        for (vec_each(val->via.array, ti_val_t, v))
        {
            if (ti_val_to_packer(v, packer, pack))
                return -1;
        }
        return qp_close_array(*packer);
    case TI_VAL_THING:
        return pack == TI_VAL_PACK_NEW
                ? (val->via.thing->flags & TI_THING_FLAG_NEW
                    ? ti_thing_to_packer(val->via.thing, packer, pack)
                    : ti_thing_id_to_packer(val->via.thing, packer))
                : (val->flags & TI_VAL_FLAG_FETCH
                    ? ti_thing_to_packer(val->via.thing, packer, pack)
                    : ti_thing_id_to_packer(val->via.thing, packer));
    case TI_VAL_THINGS:
        if (qp_add_array(packer))
            return -1;
        for (vec_each(val->via.things, ti_thing_t, thing))
        {
            if (    pack == TI_VAL_PACK_NEW &&
                    (val->via.thing->flags & TI_THING_FLAG_NEW))
            {
                if (ti_thing_to_packer(val->via.thing, packer, pack))
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
    case TI_VAL_UNDEFINED:
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
    case TI_VAL_NAME:
        return qp_fadd_raw(
                f,
                (uchar *) val->via.name->str,
                val->via.name->n);
    case TI_VAL_RAW:
        return qp_fadd_raw(f, val->via.raw->data, val->via.raw->n);
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
        return qp_fadd_int64(f, (int64_t) val->via.thing->id);
    case TI_VAL_THINGS:
        if (qp_fadd_type(f, QP_ARRAY_OPEN))
            return -1;
        for (vec_each(val->via.things, ti_thing_t, thing))
        {
            if (qp_fadd_int64(f, (int64_t) thing->id))
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
    case TI_VAL_UNDEFINED:          return "undefined";
    case TI_VAL_NIL:                return "nil";
    case TI_VAL_INT:                return "int";
    case TI_VAL_FLOAT:              return "float";
    case TI_VAL_BOOL:               return "bool";
    case TI_VAL_NAME:
    case TI_VAL_RAW:                return "raw";
    case TI_VAL_TUPLE:              return "tuple";
    case TI_VAL_ARRAY:
    case TI_VAL_THINGS:             return "array";
    case TI_VAL_THING:              return "thing";
    case TI_VAL_ARROW:              return "arrow-function";
    }
    assert (0);
    return "unknown";
}
