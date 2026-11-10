/*
 * ti/row.h
 */
#ifndef TI_ROW_H_
#define TI_ROW_H_

typedef struct ti_row_s  ti_row_t;

#include <inttypes.h>
#include <util/vec.h>
#include <ex.h>


struct ti_row_s
{
    vec_t * columns;  /* ti_column_t */
};

#endif  /* TI_ROW_H_ */
