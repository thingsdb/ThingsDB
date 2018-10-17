/*
 * val.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti/val.h>
#include <util/logger.h>

ti_val_t * ti_val_create(ti_val_e tp, void * v)
{
    ti_val_t * val = (ti_val_t *) malloc(sizeof(ti_val_t));
    if (!val)
        return NULL;
    if (ti_val_set(val, tp, v))
    {
        ti_val_destroy(val);
        return NULL;
    }
    return val;
}

ti_val_t * ti_val_weak_create(ti_val_e tp, void * v)
{
    ti_val_t * val = (ti_val_t *) malloc(sizeof(ti_val_t));
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

void ti_val_weak_set(ti_val_t * val, ti_val_e tp, void * v)
{
    val->tp = tp;
    switch(tp)
    {
    case TI_VAL_NIL:
        val->via.nil_ = NULL;
        break;
    case TI_VAL_INT:
    {
        int64_t * p = (int64_t *) v;
        val->via.int_ = *p;
    } break;
    case TI_VAL_FLOAT:
    {
        double * p = (double *) v;
        val->via.float_ = *p;
    } break;
    case TI_VAL_BOOL:
    {
        _Bool * p = (_Bool *) v;
        val->via.bool_ = *p;
    } break;
    case TI_VAL_RAW:
        val->via.raw_ = (ti_raw_t *) v;
        break;
    case TI_VAL_PRIMITIVES:
        val->via.primitives_ = (vec_t *) v;
        break;
    case TI_VAL_ELEM:
        val->via.thing_ = (ti_thing_t *) v;
        break;
    case TI_VAL_ELEMS:
        val->via.things_ = (vec_t *) v;
        break;
    default:
        log_critical("unknown type: %d", tp);
        assert (0);
    }
}

int ti_val_set(ti_val_t * val, ti_val_e tp, void * v)
{
    val->tp = tp;
    switch(tp)
    {
    case TI_VAL_NIL:
        val->via.nil_ = NULL;
        break;
    case TI_VAL_INT:
    {
        int64_t * p = (int64_t *) v;
        val->via.int_ = *p;
    } break;
    case TI_VAL_FLOAT:
    {
        double * p = (double *) v;
        val->via.float_ = *p;
    } break;
    case TI_VAL_BOOL:
    {
        _Bool * p = (_Bool *) v;
        val->via.bool_ = *p;
    } break;
    case TI_VAL_RAW:
        val->via.raw_ = ti_raw_dup((ti_raw_t *) v);
        if (!val->via.raw_)
        {
            val->tp = TI_VAL_NIL;
            return -1;
        }
        break;
    case TI_VAL_PRIMITIVES:
        val->via.primitives_ = vec_dup((vec_t *) v);
        if (!val->via.primitives_)
        {
            val->tp = TI_VAL_NIL;
            return -1;
        }
        break;
    case TI_VAL_ELEM:
        val->via.thing_ = ti_thing_grab((ti_thing_t *) v);
        break;
    case TI_VAL_ELEMS:
        val->via.things_ = vec_dup((vec_t *) v);
        // TODO: I think we should increase each reference in v.
        if (!val->via.things_)
        {
            val->tp = TI_VAL_NIL;
            return -1;
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
        break;
    case TI_VAL_RAW:
        free(val->via.raw_);
        break;
    case TI_VAL_PRIMITIVES:
        vec_destroy(val->via.primitives_, (vec_destroy_cb) ti_val_destroy);
        break;
    case TI_VAL_ELEM:
        ti_thing_drop(val->via.thing_);
        break;
    case TI_VAL_ELEMS:
        vec_destroy(val->via.things_, (vec_destroy_cb) ti_thing_drop);
        break;
    }
}

int ti_val_to_packer(ti_val_t * val, qp_packer_t * packer)
{
    switch (val->tp)
    {
    case TI_VAL_NIL:
        return qp_add_null(packer);
    case TI_VAL_INT:
        return qp_add_int64(packer, val->via.int_);
    case TI_VAL_FLOAT:
        return qp_add_double(packer, val->via.float_);
    case TI_VAL_BOOL:
        return val->via.bool_ ?
                qp_add_true(packer) : qp_add_false(packer);
    case TI_VAL_RAW:
        return qp_add_raw(packer, val->via.raw_->data, val->via.raw_->n);
    case TI_VAL_PRIMITIVES:
        if (qp_add_array(&packer))
            return -1;
        for (vec_each(val->via.primitives_, ti_val_t, v))
        {
            if (ti_val_to_packer(v, packer))
                return -1;
        }
        return qp_close_array(packer);
    case TI_VAL_ELEM:
        return ti_thing_id_to_packer(val->via.thing_, packer);
    case TI_VAL_ELEMS:
        if (qp_add_array(&packer))
            return -1;
        for (vec_each(val->via.things_, ti_thing_t, el))
        {
            if (ti_thing_id_to_packer(el, packer))
                return -1;
        }
        return qp_close_array(packer);
    }

    assert(0);
    return -1;
}

int ti_val_to_file(ti_val_t * val, FILE * f)
{
    assert (val && f);

    switch (val->tp)
    {
    case TI_VAL_NIL:
        return qp_fadd_type(f, QP_NULL);
    case TI_VAL_INT:
        return qp_fadd_int64(f, val->via.int_);
    case TI_VAL_FLOAT:
        return qp_fadd_double(f, val->via.float_);
    case TI_VAL_BOOL:
        return qp_fadd_type(f, val->via.bool_ ? QP_TRUE : QP_FALSE);
    case TI_VAL_RAW:
        return qp_fadd_raw(f, val->via.raw_->data, val->via.raw_->n);
    case TI_VAL_PRIMITIVES:
        if (qp_fadd_type(f, QP_ARRAY_OPEN))
            return -1;
        for (vec_each(val->via.things_, ti_val_t, v))
        {
            if (ti_val_to_file(v, f))
                return -1;
        }
        return qp_fadd_type(f, QP_ARRAY_CLOSE);
    case TI_VAL_ELEM:
        return qp_fadd_int64(f, (int64_t) val->via.thing_->id);
    case TI_VAL_ELEMS:
        if (qp_fadd_type(f, QP_ARRAY_OPEN))
            return -1;
        for (vec_each(val->via.things_, ti_thing_t, thing))
        {
            if (qp_fadd_int64(f, (int64_t) thing->id))
                return -1;
        }
        return qp_fadd_type(f, QP_ARRAY_CLOSE);
    }

    assert (0);
    return -1;
}
