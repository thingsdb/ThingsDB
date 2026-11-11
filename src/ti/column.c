/*
 * ti/column.c
 */
#include <assert.h>
#include <ti/column.h>


ti_column_t * ti_column_idname(ti_name_t * idname)
{
    ti_column_t * column = malloc(sizeof(ti_columnt_t));
    if (!column)
        return NULL;
    column->tp = TI_COLUMN_ID;
    column->via.idname = idname;
    return column;
}
ti_column_t * ti_column_field(ti_field_t * field)
{
    ti_column_t * column = malloc(sizeof(ti_columnt_t));
    if (!column)
        return NULL;
    column->tp = TI_COLUMN_FIELD;
    column->via.field = field;
    return column;

}
ti_column_t * ti_column_method(ti_method_t * method)
{
    ti_column_t * column = malloc(sizeof(ti_columnt_t));
    if (!column)
        return NULL;
    column->tp = TI_COLUMN_METHOD;
    column->via.method = method;
    return column;
}

ti_name_t * ti_column_name(ti_column_t * column)
{
    switch (column->tp)
    {
    case TI_COLUMN_ID:
        return column->via.idname;
    case TI_COLUMN_FIELD:
        return column->via.field->name;
    case TI_COLUMN_METHOD:
        return column->via.method->name;
    }
    assert (0);
    return NULL;
}