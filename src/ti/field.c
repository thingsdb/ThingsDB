/*
 * ti/field.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/field.h>

#define TI_SPEC_DOC_ TI_SEE_DOC("#type-declaration")

ti_field_t * ti_create_field(ti_name_t * name, ti_raw_t * spec, ex_t * e)
{
    ti_field_flag_enum_t flags = 0;
    ti_field_t field;
    size_t n = spec->n;

    if (!n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid type declaration; "
                "type declarations must not be empty"TI_SPEC_DOC_);
        return NULL;
    }

    if (spec->data[n-1] == '?')
    {
        flags |= TI_FIELD_FLAG_NULLABLE;
        --n;
    }





    field = malloc(sizeof(ti_field_t));
    if (!field)
    {
        ex_set_mem(e);
        return NULL;
    }

    field->name = name;
    field->spec = spec;

    ti_incref(name);
    ti_incref(spec);

    return field;
}


void ti_field_destroy(ti_field_t * field)
{
    if (!field)
        return;

    ti_name_drop(field->name);
    ti_val_drop(field->spec);

    free(field);
}


/*
 * Returns 0 if the given value is valid for this field
 */
int ti_field_check(ti_field_t * field, ti_val_t * val, ex_t * e)
{
    if (field->flags & TI_FIELD_FLAG_ANY)
        return 0;

    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        if (~field->flags & TI_FIELD_FLAG_NULLABLE)

    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_QP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:

    }

type_error:
    ex_set(e, EX_TYPE_ERROR,
            "type `%s` is invalid for property `%s` with definition `%s`",
            field->name->str,
            ti_val)
}
