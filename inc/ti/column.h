/*
 * ti/column.h
 */
#ifndef TI_COLUMN_H_
#define TI_COLUMN_H_

typedef struct ti_column_s  ti_column_t;

#include <inttypes.h>
#include <util/vec.h>
#include <ti/field.t.h>
#include <ti/method.t.h>
#include <ex.h>

typedef enum
{
    TI_COLUMN_ID,
    TI_COLUMN_FIELD,
    TI_COLUMN_METHOD,
} ti_column_enum_t

typedef union
{
    ti_field_t * field;
    ti_method_t * method;
} ti_column_via_t;

struct ti_column_s
{
    ti_column_enum_t tp;
    ti_column_via_t via;
};

#endif  /* TI_COLUMN_H_ */
