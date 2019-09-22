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
        if (field->flags & TI_FIELD_FLAG_NULLABLE)
            return 0;
        goto type_error;
    case TI_VAL_INT:
        if (field->tp == TI_VAL_INT || (field->flags & TI_FIELD_FLAG_NUMBER))
            return 0;
        goto type_error;
    case TI_VAL_FLOAT:
        if (field->tp == TI_VAL_FLOAT || (field->flags & TI_FIELD_FLAG_NUMBER))
            return 0;
        goto type_error;
    case TI_VAL_BOOL:
        if (field->tp == TI_VAL_BOOL)
            return 0;
        goto type_error;
    case TI_VAL_QP:
        goto type_error;
    case TI_VAL_NAME:
        if (field->tp == TI_VAL_RAW)
            return 0;
        goto type_error;
    case TI_VAL_RAW:
        if (field->tp != TI_VAL_RAW)
            goto type_error;
        if ((field->flags & TI_FIELD_FLAG_UTF8) &&
            !strx_is_utf8n(
                    (const char *) ((ti_raw_t *) val)->data,
                    ((ti_raw_t *) val)->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "property `%s` only accepts valid UTF8 data",
                    field->name->str);
            return e->nr;
        }
        return 0;
    case TI_VAL_REGEX:
        goto type_error;
    case TI_VAL_THING:
        if (field->tp != TI_VAL_THING || (
                field->class != ((ti_thing_t *) val)->class &&
                field->class != TI_OBJECT_CLASS))
            goto type_error;
        return 0;
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        goto type_error;

    }
    return 0;

type_error:
    ex_set(e, EX_TYPE_ERROR,
            "type `%s` is invalid for property `%s` with definition `%.*s`",
            ti_val_str(val),
            field->name->str,
            (int) field->spec->n,
            (const char *) field->spec->data);
    return e->nr;
}
