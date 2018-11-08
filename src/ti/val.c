/*
 * val.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti/val.h>
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

void ti_val_weak_destroy(ti_val_t * val)
{
    if (!val)
        return;
    free(val);
}

void ti_val_weak_set(ti_val_t * val, ti_val_enum tp, void * v)
{
    val->tp = tp;
    val->flags = 0;
    switch(tp)
    {
    case TI_VAL_VAL:
        val->via.val = v;
        break;
    case TI_VAL_UNDEFINED:
        val->via.undefined = NULL;
        break;
    case TI_VAL_NIL:
        val->via.nil = NULL;
        break;
    case TI_VAL_INT:
        {
            int64_t * p = v;
            val->via.int_ = *p;
        }
        break;
    case TI_VAL_FLOAT:
        {
            double * p = v;
            val->via.float_ = *p;
        }
        break;
    case TI_VAL_BOOL:
        {
            _Bool * p = v;
            val->via.bool_ = *p;
        }
        break;
    case TI_VAL_RAW:
        val->via.raw = v;
        break;
    case TI_VAL_ARRAY:
        val->via.array = v;
        break;
    case TI_VAL_THING:
        val->via.thing = v;
        break;
    case TI_VAL_THINGS:
        val->via.things = v;
        break;
    default:
        log_critical("unknown type: %d", tp);
        assert (0);
    }
}

int ti_val_set(ti_val_t * val, ti_val_enum tp, void * v)
{
    val->tp = tp;
    val->flags = 0;
    switch(tp)
    {
    case TI_VAL_VAL:
        val->via.val = malloc(sizeof(ti_val_t));
        if (    !val->via.val ||
                ti_val_copy(val->via.val, ((ti_val_t *) v)->via.val))
        {
            val->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        break;
    case TI_VAL_UNDEFINED:
        val->via.undefined = NULL;
        break;
    case TI_VAL_NIL:
        val->via.nil = NULL;
        break;
    case TI_VAL_INT:
        {
            int64_t * p = v;
            val->via.int_ = *p;
        }
        break;
    case TI_VAL_FLOAT:
        {
            double * p = v;
            val->via.float_ = *p;
        }
        break;
    case TI_VAL_BOOL:
        {
            _Bool * p = v;
            val->via.bool_ = *p;
        }
        break;
    case TI_VAL_RAW:
        val->via.raw = ti_raw_dup((ti_raw_t *) v);
        if (!val->via.raw)
        {
            val->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        break;
    case TI_VAL_ARRAY:
        val->via.array = vec_dup((vec_t *) v);
        if (!val->via.array)
        {
            val->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        break;
    case TI_VAL_THING:
        val->via.thing = ti_grab((ti_thing_t *) v);
        break;
    case TI_VAL_THINGS:
        val->via.things = vec_dup((vec_t *) v);
        if (!val->via.things)
        {
            val->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        for (vec_each(val->via.things, ti_thing_t, thing))
            ti_incref(thing);
        break;
    }
    return 0;
}

void ti_val_weak_copy(ti_val_t * to, ti_val_t * from)
{
    to->tp = from->tp;
    to->flags = from->flags;;
    to->via = from->via;
}

int ti_val_copy(ti_val_t * to, ti_val_t * from)
{
    to->tp = from->tp;
    to->flags = from->flags;
    switch(from->tp)
    {
    case TI_VAL_VAL:
        to->via.val = malloc(sizeof(ti_val_t));
        if (    !to->via.val ||
                ti_val_copy(to->via.val, ((ti_val_t *) from)->via.val))
        {
            to->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        break;
    case TI_VAL_UNDEFINED:
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        to->via = from->via;
        break;
    case TI_VAL_RAW:
        to->via.raw = ti_raw_dup(from->via.raw);
        if (!to->via.raw)
        {
            to->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        break;
    case TI_VAL_ARRAY:
        to->via.array = vec_dup(from->via.array);
        if (!to->via.array)
        {
            to->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        break;
    case TI_VAL_THING:
        to->via.thing = ti_grab(from->via.thing);
        break;
    case TI_VAL_THINGS:
        to->via.things = vec_dup(from->via.things);
        if (!to->via.things)
        {
            to->tp = TI_VAL_UNDEFINED;
            return -1;
        }
        for (vec_each(to->via.things, ti_thing_t, thing))
            ti_incref(thing);
        break;
    }
    return 0;
}

void ti_val_set_bool(ti_val_t * val, _Bool bool_)
{
    val->tp = TI_VAL_BOOL;
    val->flags = 0;
    val->via.bool_ = bool_;
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
            if (val->via.thing->id)
            {
                ti_thing_unmark_new(val->via.thing);
                continue;
            }
            if (ti_thing_gen_id(val->via.thing))
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
    switch(val->tp)
    {
    case TI_VAL_VAL:
        ti_val_destroy(val->via.val);
        break;
    case TI_VAL_UNDEFINED:
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
        break;
    case TI_VAL_RAW:
        ti_raw_free(val->via.raw);
        break;
    case TI_VAL_ARRAY:
        vec_destroy(val->via.array, (vec_destroy_cb) ti_val_destroy);
        break;
    case TI_VAL_THING:
        ti_thing_drop(val->via.thing);
        break;
    case TI_VAL_THINGS:
        vec_destroy(val->via.things, (vec_destroy_cb) ti_thing_drop);
        break;
    }
    val->flags = 0;
    val->tp = TI_VAL_UNDEFINED;
}

int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer, int pack)
{
    switch (val->tp)
    {
    case TI_VAL_VAL:
        return ti_val_to_packer(val->via.val, packer, pack);
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
    case TI_VAL_RAW:
        return qp_add_raw(*packer, val->via.raw->data, val->via.raw->n);
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
    }

    assert(0);
    return -1;
}

int ti_val_to_file(ti_val_t * val, FILE * f)
{
    assert (val && f);

    switch (val->tp)
    {
    case TI_VAL_VAL:
        return ti_val_to_file(val->via.val, f);
    case TI_VAL_UNDEFINED:
        assert (0);
        return 0;
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
    }

    assert (0);
    return -1;
}

const char * ti_val_to_str(ti_val_t * val)
{
    switch (val->tp)
    {
    case TI_VAL_VAL:                return ti_val_to_str(val->via.val);
    case TI_VAL_UNDEFINED:          return "undefined";
    case TI_VAL_NIL:                return "nil";
    case TI_VAL_INT:                return "int";
    case TI_VAL_FLOAT:              return "float";
    case TI_VAL_BOOL:               return "bool";
    case TI_VAL_RAW:                return "raw";
    case TI_VAL_ARRAY:
    case TI_VAL_THINGS:             return "array";
    case TI_VAL_THING:              return "thing";
    }
    assert (0);
    return "unknown";
}
