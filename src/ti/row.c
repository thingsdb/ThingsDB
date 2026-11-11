/*
 * ti/row.c
 */
#include <assert.h>
#include <ti/column.h>
#include <ti/row.h>
#include <ti/row.t.h>
#include <util/vec.h>


ti_row_t * ti_row_create(size_t n)
{
    ti_row_t * row = malloc(sizeof(ti_row_t));
    if (!row)
        return NULL;

    row->columns = vec_new(n);
    if (!row->columns)
    {
        ti_row_destroy(row);
        return NULL;
    }
    return row;
}

void ti_row_destroy(ti_row_t * row)
{
    if (!row)
        return;
    vec_destroy(row->columns, free);
    free(row);
}

int ti_row_to_pk(ti_row_t * row, msgpack_packer * pk)
{
    if (!row)
        return msgpack_pack_nil(pk);

    if (msgpack_pack_array(pk, row->columns->n))
        return -1;

    for (vec_each(row->columns, ti_column_t, column))
    {
        ti_name_t * name = ti_column_name(column);
        if (mp_pack_strn(pk, name->str, name->n))
            return -1;
    }
    return 0;
}

ti_column_t * ti_row_column_by_name(ti_row_t * row, ti_name_t * name)
{
    if (!row)
        return NULL;

    for (vec_each(row->columns, ti_column_t, column))
    {
        switch (column->tp)
        {
        case TI_COLUMN_ID:
            if (column->via.idname == name)
                return column;
            break;
        case TI_COLUMN_FIELD:
            if (column->via.field->name == name)
                return column;
            break;
        case TI_COLUMN_METHOD:
            if (column->via.method->name == name)
                return column;
            break;
        }
    }
    return NULL;
}
