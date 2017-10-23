/*
 * val.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <string.h>
#include <rql/val.h>

void rql_val_init(rql_val_t * val, rql_val_e tp, void * v)
{
    switch(tp)
    {
    case RQL_VAL_ELEM:  /* a value by itself has no reference */
        val->via.elem_ = (rql_elem_t *) v;
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
    {
        const char * s = (const char *) v;
        val->via.str_ = strdup(s);
        if (!val->via.str_)
        {
            free(val);
        }
    } break;
    case RQL_VAL_RAW:
        val->via.raw_ = (rql_raw_t *) v;
        break;

    case RQL_VAL_ARR:
        val->via.arr_ = (vec_t *) v;
        break;
    }

}

/*
 * Clear value
 */
void rql_val_clear(rql_val_t * val)
{
    switch(val->tp)
    {
    case RQL_VAL_ELEM:
    case RQL_VAL_INT:
    case RQL_VAL_FLOAT:
    case RQL_VAL_BOOL:
        break;;
    case RQL_VAL_STR:
        free(val->via.str_);
        break;
    case RQL_VAL_RAW:
        free(val->via.raw_);
        break;
    case RQL_VAL_ARR:
        break;
    }
}
