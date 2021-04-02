#include <ti/enum.t.h>
#include <ti/enums.t.h>
#include <ti/export.h>
#include <ti/field.h>
#include <ti/field.t.h>
#include <ti/fmt.h>
#include <ti/member.t.h>
#include <ti/method.t.h>
#include <ti/procedure.h>
#include <ti/raw.inline.h>
#include <ti/type.t.h>
#include <ti/types.t.h>
#include <ti/val.inline.h>
#include <ti/val.t.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/smap.h>
#include <util/vec.h>

static int export__new_type_cb(ti_type_t * type_, ti_fmt_t * fmt)
{
    return (
        buf_append_str(&fmt->buf, "new_type('") ||
        buf_append(
                &fmt->buf,
                (const char *) type_->rname->data,
                type_->rname->n) ||
        (type_->flags & TI_TYPE_FLAG_WRAP_ONLY)
          ? buf_append_str(&fmt->buf, "', true);\n")
          : buf_append_str(&fmt->buf, "');\n")
    );
}

static int export__set_type_cb(ti_type_t * type_, ti_fmt_t * fmt)
{
    int spaces;
    if (type_->fields->n == 0 && type_->methods->n == 0)
        return 0;

    if (
        buf_append_str(&fmt->buf, "set_type('") ||
        buf_append(
                &fmt->buf,
                (const char *) type_->rname->data,
                type_->rname->n) ||
        buf_append_str(&fmt->buf, "', {\n")
    ) return -1;

    ++fmt->indent;
    spaces = fmt->indent * fmt->indent_spaces;

    /*
     * TOFO: test quotes etc. in definition
     */
    for (vec_each(type_->fields, ti_field_t, field))
    {
        if (buf_append_fmt(&fmt->buf, "%*s", spaces, "") ||
            buf_append(&fmt->buf, field->name->str, field->name->n) ||
            buf_append_str(&fmt->buf, ": ") ||
            ti_fmt_ti_string(fmt, field->spec_raw) ||
            buf_append_str(&fmt->buf, ",\n")
        ) return -1;
    }

    for (vec_each(type_->methods, ti_method_t, method))
    {
        if (buf_append_fmt(&fmt->buf, "%*s", spaces, "") ||
            buf_append(&fmt->buf, method->name->str, method->name->n) ||
            buf_append_str(&fmt->buf, ": ") ||
            ti_fmt_nd(fmt, method->closure->node) ||
            buf_append_str(&fmt->buf, ",\n")
        ) return -1;
    }

    --fmt->indent;
    return buf_append_str(&fmt->buf, "});\n");
}

static int export__write_relation(
        ti_fmt_t * fmt,
        ti_field_t * field_a,
        ti_field_t * field_b)
{
    return -(
        buf_append_str(&fmt->buf, "mod_type('") ||
        buf_append(
                &fmt->buf,
                (const char *) field_a->type->rname->data,
                field_a->type->rname->n) ||
        buf_append_str(&fmt->buf, "', 'rel', '") ||
        buf_append(&fmt->buf, field_a->name->str, field_a->name->n) ||
        buf_append_str(&fmt->buf, "', '") ||
        buf_append(&fmt->buf, field_b->name->str, field_b->name->n) ||
        buf_append_str(&fmt->buf, "');\n")
    );
}

static int export__relation_cb(ti_type_t * type_, ti_fmt_t * fmt)
{
    for (vec_each(type_->fields, ti_field_t, field))
    {
        if (ti_field_has_relation(field))
        {
            ti_field_t * other = field->condition.rel->field;
            if ((   field == other ||
                    field->type->type_id < other->type->type_id ||
                    (   field->type->type_id == other->type->type_id &&
                        field->idx < other->idx)) &&
                    export__write_relation(fmt, field, other))
                return -1;
        }
    }
    return 0;
}

static int export__write_types(ti_fmt_t * fmt, ti_types_t * types)
{
    return (
        buf_append_str(&fmt->buf,
"\n"
"/*\n"
" * Types\n"
" */\n"
"\n") ||
        smap_values(types->smap, (smap_val_cb) export__new_type_cb, fmt) ||
        buf_write(&fmt->buf, '\n') ||
        smap_values(types->smap, (smap_val_cb) export__set_type_cb, fmt) ||
        buf_write(&fmt->buf, '\n') ||
        smap_values(types->smap, (smap_val_cb) export__relation_cb, fmt)
    );
}

static int export__set_enum_cb(ti_enum_t * enum_, ti_fmt_t * fmt)
{
    if (enum_->members->n)
    {
        ti_member_t * member = vec_first(enum_->members);
        if (ti_val_is_thing(member->val))
        {
            buf_append_str(&fmt->buf,
"\n"
"/*\n"
" * WARNING: The following enumerator is of type `things` and since data is not\n"
" *          exported by this function, the enumerator is created with empty\n"
" *          things instead!\n"
" */\n"
);
        }
    }

    if (
        buf_append_str(&fmt->buf, "set_enum('") ||
        buf_append(
                &fmt->buf,
                (const char *) enum_->rname->data,
                enum_->rname->n) ||
        buf_append_str(&fmt->buf, "', {")
    ) return -1;

    if (enum_->members->n)
    {
        int spaces;

        ++fmt->indent;
        spaces = fmt->indent * fmt->indent_spaces;

        if (buf_write(&fmt->buf, '\n'))
            return -1;

        for (vec_each(enum_->members, ti_member_t, member))
        {
            if (buf_append_fmt(&fmt->buf, "%*s", spaces, "") ||
                buf_append(&fmt->buf, member->name->str, member->name->n) ||
                buf_append_str(&fmt->buf, ": ")
            ) return -1;

            switch((ti_val_enum) member->val->tp)
            {
            case TI_VAL_NIL:
            case TI_VAL_BOOL:
            case TI_VAL_DATETIME:
            case TI_VAL_REGEX:
            case TI_VAL_WRAP:
            case TI_VAL_ARR:
            case TI_VAL_SET:
            case TI_VAL_CLOSURE:
            case TI_VAL_ERROR:
            case TI_VAL_MEMBER:
            case TI_VAL_FUTURE:
            case TI_VAL_TEMPLATE:
                assert(0);
                break;
            case TI_VAL_INT:
                if (buf_append_fmt(
                        &fmt->buf,
                        "%"PRId64",\n",
                        VINT(member->val)))
                    return -1;
                break;
            case TI_VAL_FLOAT:
                if (buf_append_fmt(
                        &fmt->buf,
                        "%f,\n",
                        VFLOAT(member->val)))
                    return -1;
                break;
            case TI_VAL_MP:
            case TI_VAL_STR:
            case TI_VAL_NAME:
                if (ti_fmt_ti_string(fmt, (ti_raw_t *) member->val) ||
                    buf_append_str(&fmt->buf, ",\n"))
                    return -1;
                break;
            case TI_VAL_BYTES:
                {
                    int rc;
                    ti_raw_t * tmp = ti_str_base64_from_raw(
                            (ti_raw_t *) member->val);
                    if (!tmp)
                        return -1;
                    rc = buf_append_fmt(
                            &fmt->buf,
                            "base64_decode('%.*s'),\n",
                            (int) tmp->n,
                            (const char *) tmp->data);
                    ti_val_unsafe_drop((ti_val_t *) tmp);
                    if (rc)
                        return -1;
                }
                break;
            case TI_VAL_THING:
                /*
                 * Things are create as EMPTY things; Data is not exported
                 */
                if (buf_append_str(&fmt->buf, "{},\n"))
                    return -1;
                break;

            }
        }

        --fmt->indent;
    }

    buf_append_str(&fmt->buf, "});\n");
    return 0;
}

static int export__write_enums(ti_fmt_t * fmt, ti_enums_t * enums)
{
    return (
        buf_append_str(&fmt->buf,
"/*\n"
" * Enums\n"
" */\n"
"\n") ||
        smap_values(enums->smap, (smap_val_cb) export__set_enum_cb, fmt)
    );

}

static int export__procedure_cb(ti_procedure_t * procedure, ti_fmt_t * fmt)
{
    return -(
        buf_append_str(&fmt->buf, "new_procedure('") ||
        buf_append(
                &fmt->buf,
                procedure->name,
                procedure->name_n) ||
        buf_append_str(&fmt->buf, "', ") ||
        ti_fmt_nd(fmt, procedure->closure->node) ||
        buf_append_str(&fmt->buf, ");\n")
    );
}

static int export__write_procedures(ti_fmt_t * fmt, smap_t * procedures)
{
    if (buf_append_str(&fmt->buf,
"\n"
"/*\n"
" * Procedures\n"
" */\n"
"\n")) return -1;

    return smap_values(procedures, (smap_val_cb) export__procedure_cb, fmt);
}

static int export__collection(ti_fmt_t * fmt, ti_collection_t * collection)
{
    return (
            export__write_enums(fmt, collection->enums) ||
            export__write_types(fmt, collection->types) ||
            export__write_procedures(fmt, collection->procedures) ||
            buf_append_str(&fmt->buf, "\n'DONE';\n")
    );
}

/*
 * Export enumerators, types and procedures. (No data will be exported and
 * enumerators of type `thing` are created with new *empty* things.
 */
ti_raw_t * ti_export_collection(ti_collection_t * collection)
{
    ti_raw_t * str;
    ti_fmt_t fmt;
    ti_fmt_init(&fmt, FMT_INDENT);

    if (export__collection(&fmt, collection))
        return NULL;

    str = ti_str_create(fmt.buf.data, fmt.buf.len);
    ti_fmt_clear(&fmt);
    return str;
}

