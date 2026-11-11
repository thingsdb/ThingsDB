/*
 * ti/row.h
 */
#ifndef TI_ROW_H_
#define TI_ROW_H_

#include <inttypes.h>
#include <ti/column.h>
#include <ti/field.t.h>
#include <ti/method.t.h>
#include <ti/name.t.h>
#include <ti/row.t.h>
#include <ti/type.t.h>
#include <util/mpack.h>
#include <util/vec.h>
#include <ex.h>

ti_row_t * ti_row_create(size_t n);
void ti_row_destroy(ti_row_t * row);
int ti_row_to_pk(ti_row_t * row, msgpack_packer * pk);
ti_column_t * ti_row_column_by_name(ti_row_t * row, ti_name_t * name)

#endif  /* TI_ROW_H_ */
