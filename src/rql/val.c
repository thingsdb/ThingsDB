/*
 * val.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <rql/val.h>
#include <util/logger.h>

rql_val_t * rql_val_create(rql_val_e tp, void * v)
{
    rql_val_t * val = (rql_val_t *) malloc(sizeof(rql_val_t));
    if (!val) return NULL;
    if (rql_val_set(val, tp, v))
    {
        rql_val_destroy(val);
        return NULL;
    }
    return val;
}

rql_val_t * rql_val_weak_create(rql_val_e tp, void * v)
{
    rql_val_t * val = (rql_val_t *) malloc(sizeof(rql_val_t));
    if (!val) return NULL;
    rql_val_weak_set(val, tp, v);
    return val;
}

void rql_val_destroy(rql_val_t * val)
{
    if (!val) return;
    rql_val_clear(val);
    free(val);
}

void rql_val_weak_set(rql_val_t * val, rql_val_e tp, void * v)
{
    val->tp = tp;
    switch(tp)
    {
    case RQl_VAL_NIL:
        val->via.nil_ = NULL;
        break;
    case RQL_VAL_INT:
    {
        int64_t * p = (int64_t *) v;
        val->via.int_ = *p;
    } break;
    case RQL_VAL_FLOAT:
    {
        double * p = (double *) v;
        val->via.float_ = *p;
    } break;
    case RQL_VAL_BOOL:
    {
        _Bool * p = (_Bool *) v;
        val->via.bool_ = *p;
    } break;
    case RQL_VAL_STR:
        val->via.str_ = (char *) v;
        break;
    case RQL_VAL_RAW:
        val->via.raw_ = (rql_raw_t *) v;
        break;
    case RQL_VAL_ELEM:
        val->via.elem_ = (rql_elem_t *) v;
        break;
    case RQL_VAL_ELEMS:
        val->via.elems_ = (vec_t *) v;
        break;
    case RQL_VAL_PRIMITIVES:
        val->via.primitives_ = (vec_t *) v;
        break;
    }
}

int rql_val_set(rql_val_t * val, rql_val_e tp, void * v)
{
    val->tp = tp;
    switch(tp)
    {
    case RQl_VAL_NIL:
        val->via.nil_ = NULL;
        break;
    case RQL_VAL_INT:
    {
        int64_t * p = (int64_t *) v;
        val->via.int_ = *p;
    } break;
    case RQL_VAL_FLOAT:
    {
        double * p = (double *) v;
        val->via.float_ = *p;
    } break;
    case RQL_VAL_BOOL:
    {
        _Bool * p = (_Bool *) v;
        val->via.bool_ = *p;
    } break;
    case RQL_VAL_STR:
        val->via.str_ = strdup((const char *) v);
        if (!val->via.str_)
        {
            val->tp = RQl_VAL_NIL;
            return -1;
        }
        break;
    case RQL_VAL_RAW:
        val->via.raw_ = rql_raw_dup((rql_raw_t *) v);
        if (!val->via.raw_)
        {
            val->tp = RQl_VAL_NIL;
            return -1;
        }
        break;
    case RQL_VAL_ELEM:
        val->via.elem_ = rql_elem_grab((rql_elem_t *) v);
        break;
    case RQL_VAL_ELEMS:
        val->via.elems_ = vec_dup((vec_t *) v);
        if (!val->via.elems_)
        {
            val->tp = RQl_VAL_NIL;
            return -1;
        }
        break;
    case RQL_VAL_PRIMITIVES:
        val->via.primitives_ = vec_dup((vec_t *) v);
        if (!val->via.primitives_)
        {
            val->tp = RQl_VAL_NIL;
            return -1;
        }
        break;
    }
    return 0;
}

/*
 * Clear value
 */
void rql_val_clear(rql_val_t * val)
{
    switch(val->tp)
    {
    case RQl_VAL_NIL:
    case RQL_VAL_INT:
    case RQL_VAL_FLOAT:
    case RQL_VAL_BOOL:
        break;
    case RQL_VAL_STR:
        free(val->via.str_);
        break;
    case RQL_VAL_RAW:
        free(val->via.raw_);
        break;
    case RQL_VAL_ELEM:
        rql_elem_drop(val->via.elem_);
        break;
    case RQL_VAL_ELEMS:
        vec_destroy(val->via.elems_, (vec_destroy_cb) rql_elem_drop);
        break;
    case RQL_VAL_PRIMITIVES:
        vec_destroy(val->via.primitives_, (vec_destroy_cb) rql_val_destroy);
        break;
    }
}

int rql_val_to_packer(rql_val_t * val, qp_packer_t * packer)
{
    switch (val->tp)
    {
    case RQl_VAL_NIL:
        return qp_add_null(packer);
    case RQL_VAL_INT:
        return qp_add_int64(packer, val->via.int_);
    case RQL_VAL_FLOAT:
        return qp_add_double(packer, val->via.float_);
    case RQL_VAL_BOOL:
        return val->via.bool_ ?
                qp_add_true(packer) : qp_add_false(packer);
    case RQL_VAL_STR:
        return qp_add_raw_from_str(packer, val->via.str_);
    case RQL_VAL_RAW:
        return qp_add_raw(packer, val->via.raw_->data, val->via.raw_->n);
    case RQL_VAL_ELEM:
        return rql_elem_id_to_packer(val->via.elem_, packer);
    case RQL_VAL_ELEMS:
        if (qp_add_array(&packer)) return -1;
        for (vec_each(val->via.elems_, rql_elem_t, el))
        {
            if (rql_elem_id_to_packer(el, packer)) return -1;
        }
        return qp_close_array(packer);
    case RQL_VAL_PRIMITIVES:
        if (qp_add_array(&packer)) return -1;
        for (vec_each(val->via.primitives_, rql_val_t, v))
        {
            if (rql_val_to_packer(v, packer)) return -1;
        }
        return qp_close_array(packer);
    }

    assert(0);
    return -1;
}
