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
    ti_val_weak_set(val, tp, v);
    return val;
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
    switch(tp)
    {
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
    case TI_VAL_PRIMITIVES:
        val->via.primitives = v;
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
    switch(tp)
    {
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
            val->tp = TI_VAL_NIL;
            return -1;
        }
        break;
    case TI_VAL_PRIMITIVES:
        val->via.primitives = vec_dup((vec_t *) v);
        if (!val->via.primitives)
        {
            val->tp = TI_VAL_NIL;
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
            val->tp = TI_VAL_NIL;
            return -1;
        }
        for (vec_each(val->via.things, ti_thing_t, thing))
        {
            ti_grab(thing);
        }
        break;
    }
    return 0;
}

void ti_val_weak_copy(ti_val_t * to, ti_val_t * from)
{
    to->tp = from->tp;
    to->via = from->via;
}

int ti_val_copy(ti_val_t * to, ti_val_t * from)
{
    to->tp = from->tp;
    switch(to->tp)
    {
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
            to->tp = TI_VAL_NIL;
            return -1;
        }
        break;
    case TI_VAL_PRIMITIVES:
        to->via.primitives = vec_dup(from->via.primitives);
        if (!to->via.primitives)
        {
            to->tp = TI_VAL_NIL;
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
            to->tp = TI_VAL_NIL;
            return -1;
        }
        for (vec_each(to->via.things, ti_thing_t, thing))
        {
            ti_grab(thing);
        }
        break;
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
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_UNDEFINED:
        break;
    case TI_VAL_RAW:
        ti_raw_free(val->via.raw);
        break;
    case TI_VAL_PRIMITIVES:
        vec_destroy(val->via.primitives, (vec_destroy_cb) ti_val_destroy);
        break;
    case TI_VAL_THING:
        ti_thing_drop(val->via.thing);
        break;
    case TI_VAL_THINGS:
        vec_destroy(val->via.things, (vec_destroy_cb) ti_thing_drop);
        break;
    }
    val->tp = TI_VAL_UNDEFINED;
}

int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer)
{
    switch (val->tp)
    {
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
    case TI_VAL_PRIMITIVES:
        if (qp_add_array(packer))
            return -1;
        for (vec_each(val->via.primitives, ti_val_t, v))
        {
            if (ti_val_to_packer(v, packer))
                return -1;
        }
        return qp_close_array(*packer);
    case TI_VAL_THING:
        return ti_thing_id_to_packer(val->via.thing, packer);
    case TI_VAL_THINGS:
        if (qp_add_array(packer))
            return -1;
        for (vec_each(val->via.things, ti_thing_t, el))
        {
            if (ti_thing_id_to_packer(el, packer))
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
    case TI_VAL_PRIMITIVES:
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
    case TI_VAL_UNDEFINED:          return "undefined";
    case TI_VAL_NIL:                return "nil";
    case TI_VAL_INT:                return "int";
    case TI_VAL_FLOAT:              return "float";
    case TI_VAL_BOOL:               return "bool";
    case TI_VAL_RAW:                return "raw";
    case TI_VAL_PRIMITIVES:
    case TI_VAL_THINGS:             return "array";
    case TI_VAL_THING:              return "thing";
    }
    assert (0);
    return "unknown";
}
