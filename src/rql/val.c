/*
 * val.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <string.h>
#include <rql/val.h>

rql_val_via_t * rql_val_create(rql_val_e tp, void * data)
{
    rql_val_via_t * val = (rql_val_via_t *) malloc(sizeof(rql_val_via_t));
    if (!val) return NULL;

    switch(tp)
    {
    case RQL_VAL_ELEM:  /* a value by itself has no reference */
        val->elem_ = (rql_elem_t *) data;
        break;
    case RQL_VAL_INT:
    {
        int64_t * p = (int64_t *) data;
        val->int_ = *p;
    } break;
    case RQL_VAL_FLOAT:
    {
        double * p = (double *) data;
        val->float_ = *p;
    } break;
    case RQL_VAL_BOOL:
    {
        _Bool * p = (_Bool *) data;
        val->bool_ = *p;
    } break;
    case RQL_VAL_STR:
    {
        const char * s = (const char *) data;
        val->str_ = strdup(s);
        if (!val->str_)
        {
            free(val);
            return NULL;
        }
    } break;
    case RQL_VAL_RAW:
        val->raw_ = (rql_raw_t *) data;
        break;
    }
    return val;
}

/*
 * Destroy a value.
 */
void rql_val_destroy(rql_val_e tp, rql_val_via_t * via)
{
    switch(tp)
    {
    case RQL_VAL_ELEM:  /* a value by itself has no reference */
    case RQL_VAL_INT:
    case RQL_VAL_FLOAT:
    case RQL_VAL_BOOL:
        break;;
    case RQL_VAL_STR:
        free(via->str_);
        break;
    case RQL_VAL_RAW:
        free(via->raw_);
        break;
    }
    free(via);
}
