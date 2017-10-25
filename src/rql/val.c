/*
 * val.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <string.h>
#include <rql/val.h>

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
