/*
 * ti/field.c
 */
#include <assert.h>
#include <doc.h>
#include <math.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/condition.h>
#include <ti/data.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/field.h>
#include <ti/method.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/query.h>
#include <ti/spec.h>
#include <ti/spec.inline.h>
#include <ti/thing.inline.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/watch.h>
#include <util/strx.h>

static void field__remove_dep(ti_field_t * field)
{
    ti_type_t * type;
    uint32_t idx;
    uint16_t spec;

    spec = field->spec & TI_SPEC_MASK_NILLABLE;
    if (spec < TI_SPEC_ANY)
        goto decref_type;
    if (spec >= TI_ENUM_ID_FLAG)
        goto decref_enum;

    spec = field->nested_spec & TI_SPEC_MASK_NILLABLE;
    if (spec < TI_SPEC_ANY)
        goto decref_type;
    if (spec >= TI_ENUM_ID_FLAG)
        goto decref_enum;

    return;

decref_enum:
    spec &= TI_ENUM_ID_MASK;
    type = (ti_type_t *) ti_enums_by_id(
            field->type->types->collection->enums,
            spec);
    goto decref;

decref_type:
    type = ti_types_by_id(field->type->types, spec);
    if (type == field->type)
        return;  /* self references are not counted within dependencies */

decref:
    idx = 0;
    for(vec_each(field->type->dependencies, ti_type_t, t), ++idx)
    {
        if (t == type)
        {
            vec_swap_remove(field->type->dependencies, idx);
            break;
        }
    }
    --type->refcount;
    return;
}

/* Used for detecting circular references between types */
static int field__dep_check(ti_type_t * dep, ti_type_t * type)
{
    for (vec_each(dep->fields, ti_field_t, field))
    {
        if (field->spec == type->type_id)
            return -1;

        if (field->spec < TI_SPEC_ANY &&
            field__dep_check(
                ti_types_by_id(field->type->types, field->spec),
                type)
        ) return -1;
    }
    return 0;
}

static int field__add_dep(ti_field_t * field)
{
    ti_type_t * type;
    uint16_t spec;

    spec = field->spec & TI_SPEC_MASK_NILLABLE;
    if (spec < TI_SPEC_ANY)
        goto incref_type;
    if (spec >= TI_ENUM_ID_FLAG)
        goto incref_enum;

    spec = field->nested_spec & TI_SPEC_MASK_NILLABLE;
    if (spec < TI_SPEC_ANY)
        goto incref_type;
    if (spec >= TI_ENUM_ID_FLAG)
        goto incref_enum;
    return 0;

incref_enum:
    spec &= TI_ENUM_ID_MASK;
    type = (ti_type_t *) ti_enums_by_id(
            field->type->types->collection->enums,
            spec);
    goto incref;

incref_type:
    type = ti_types_by_id(field->type->types, spec);
    if (type == field->type)
        return 0;  /* self references are not counted within dependencies */

incref:
    if (vec_push(&field->type->dependencies, type))
        return -1;

    ++type->refcount;
    return 0;
}

static _Bool field__spec_is_ascii(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{
    if (!strx_is_asciin(str, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type declarations must only contain valid ASCII characters"
                DOC_T_TYPE,
                field->name->str, field->type->name);
        return false;
    }
    return true;
}

#define field__cmp(__a, __na, __str) \
        strlen(__str) == __na && memcmp(__a, __str, __na) == 0

static ti_data_t * field___set_job(ti_name_t * name, ti_val_t * val)
{
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_data_t), sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_array(&pk, 1) ||
        msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "set") ||
        msgpack_pack_map(&pk, 1)
    ) goto fail_pack;

    if (mp_pack_strn(&pk, name->str, name->n) ||
        ti_val_to_pk(val, &pk, 0)
    ) goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    return data;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return NULL;
}

static ti_data_t * field__del_job(const char * name, size_t n)
{
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, 64 + n, sizeof(ti_data_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 1);
    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "del");
    mp_pack_strn(&pk, name, n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    return data;
}

static int field__init(ti_field_t * field, ex_t * e)
{
    const char * str = (const char *) field->spec_raw->data;
    size_t n = field->spec_raw->n;
    uint16_t * spec = &field->spec;

    field->spec = 0;
    field->nested_spec = 0;

    if (!n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type declarations must not be empty"DOC_T_TYPE,
                field->name->str, field->type->name);
        return e->nr;
    }

    if (str[n-1] == '?')
    {
        field->spec |= TI_SPEC_NILLABLE;
        if (!--n)
            goto invalid;
    }

    if (*str == '[')
    {
        if (str[n-1] != ']')
            goto invalid;
        field->spec |= TI_SPEC_ARR;
    }
    else if (*str == '{')
    {
        if (str[n-1] != '}')
            goto invalid;
        field->spec |= TI_SPEC_SET;
    }
    else
    {
        field->nested_spec = TI_SPEC_ANY;  /* must default to any */
        goto skip_nesting;
    }

    /* continue array or set definition */

    if (!(n -= 2))
    {
        field->nested_spec = TI_SPEC_ANY;  /* must default to any */
        return 0;
    }

    spec = &field->nested_spec;
    ++str;

    if (str[n-1] == '?')
    {
        *spec |= TI_SPEC_NILLABLE;
        if (!--n)
            goto invalid;
    }

    if (*str == '/')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "nested pattern conditions are not allowed"
            DOC_T_TYPE, field->name->str, field->type->name);
        return e->nr;
    }

    if (str[n-1] == '>')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "nested range conditions are not allowed"
            DOC_T_TYPE, field->name->str, field->type->name);
        return e->nr;
    }

skip_nesting:

    assert (n);

    if (*str == '[')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `[`; nested array declarations are not allowed"
            DOC_T_TYPE, field->name->str, field->type->name);
        return e->nr;
    }

    if (*str == '{')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `{`; nested set declarations are not allowed"
            DOC_T_TYPE, field->name->str, field->type->name);
        return e->nr;
    }

    if (*str != '/' && str[n-1] == '>')
        return ti_condition_field_range_init(field, str, n, e);

    switch(*str)
    {
    case '/':
        return ti_condition_field_re_init(field, str, n, e);
    case 'a':
        if (field__cmp(str, n, "any"))
        {
            *spec |= TI_SPEC_ANY;
            goto found;
        }
        break;
    case 'b':
        if (field__cmp(str, n, "bool"))
        {
            *spec |= TI_SPEC_BOOL;
            goto found;
        }
        if (field__cmp(str, n, "bytes"))
        {
            *spec |= TI_SPEC_BYTES;
            goto found;
        }
        break;
    case 'f':
        if (field__cmp(str, n, "float"))
        {
            *spec |= TI_SPEC_FLOAT;
            goto found;
        }
        break;
    case 'i':
        if (field__cmp(str, n, "int"))
        {
            *spec |= TI_SPEC_INT;
            goto found;
        }
        break;
    case 'n':
        if (field__cmp(str, n, "nint"))
        {
            *spec |= TI_SPEC_NINT;
            goto found;
        }
        if (field__cmp(str, n, "number"))
        {
            *spec |= TI_SPEC_NUMBER;
            goto found;
        }
        break;
    case 'p':
        if (field__cmp(str, n, "pint"))
        {
            *spec |= TI_SPEC_PINT;
            goto found;
        }
        break;
    case 'r':
        if (field__cmp(str, n, "raw"))
        {
            *spec |= TI_SPEC_RAW;
            goto found;
        }
        break;
    case 's':
        if (field__cmp(str, n, "str"))
        {
            *spec |= TI_SPEC_STR;
            goto found;
        }
        break;
    case 't':
        if (field__cmp(str, n, "thing"))
        {
            *spec |= TI_SPEC_OBJECT;
            goto found;
        }
        break;
    case 'u':
        if (field__cmp(str, n, "utf8"))
        {
            *spec |= TI_SPEC_UTF8;
            goto found;
        }
        if (field__cmp(str, n, "uint"))
        {
            *spec |= TI_SPEC_UINT;
            goto found;
        }
        break;
    }

    if (field__cmp(str, n, field->type->name))
    {
        *spec |= field->type->type_id;
        if (&field->spec == spec && (~field->spec & TI_SPEC_NILLABLE))
            goto circular_dep;
    }
    else
    {
        ti_type_t * dep = ti_types_by_strn(field->type->types, str, n);
        if (!dep)
        {
            dep = (ti_type_t *) ti_enums_by_strn(
                    field->type->types->collection->enums,
                    str,
                    n);

            if (!dep)
            {
                if (field__spec_is_ascii(field, str, n, e))
                    ex_set(e, EX_TYPE_ERROR,
                            "invalid declaration for `%s` on type `%s`; "
                            "unknown type `%.*s` in declaration"DOC_T_TYPE,
                            field->name->str, field->type->name,
                            (int) n, str);
                return e->nr;
            }

            if ((field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET)
            {
                ex_set(e, EX_TYPE_ERROR,
                    "invalid declaration for `%s` on type `%s`; "
                    "type `"TI_VAL_SET_S"` cannot contain enum type `%.*s`"
                    DOC_T_TYPE,
                    field->name->str, field->type->name,
                    (int) n, str);
                return e->nr;
            }

            /* When an enum is cast to a type, the enum_id becomes type_id */
            *spec |= dep->type_id | TI_ENUM_ID_FLAG;
        }
        else
        {
            *spec |= dep->type_id;



            if (&field->spec == spec && (~field->spec & TI_SPEC_NILLABLE))
            {
                if (field__dep_check(dep, field->type))
                    goto circular_dep;

                if (!ti_type_is_wrap_only(field->type) &&
                    ti_type_is_wrap_only(dep))
                {
                    ex_set(e, EX_TYPE_ERROR,
                        "invalid declaration for `%s` on type `%s`; "
                        "when depending on a type in wrap-only mode, both "
                        "types must have wrap-only mode enabled; either add "
                        "a `?` to make the dependency nillable or make `%s` a "
                        "wrap-only type as well"
                        DOC_T_TYPE,
                        field->name->str, field->type->name,
                        field->type->name);
                    return e->nr;
                }
            }
        }

        if (vec_push(&field->type->dependencies, dep))
        {
            ex_set_mem(e);
            return e->nr;
        }

        ++dep->refcount;
    }

found:
    if ((field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET)
    {
        if (field->nested_spec & TI_SPEC_NILLABLE)
           ex_set(e, EX_TYPE_ERROR,
               "invalid declaration for `%s` on type `%s`; "
               "type `"TI_VAL_SET_S"` cannot contain "
               "a nillable type"DOC_T_TYPE,
               field->name->str, field->type->name);
        else if (field->nested_spec > TI_SPEC_OBJECT)
            ex_set(e, EX_TYPE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type `"TI_VAL_SET_S"` cannot contain type `%s`"
                DOC_T_TYPE,
                field->name->str, field->type->name,
                ti__spec_approx_type_str(field->nested_spec));
        else if (field->nested_spec == TI_SPEC_ANY)
            field->nested_spec = TI_SPEC_OBJECT;
    }

    return e->nr;

invalid:
    if (!field__spec_is_ascii(
            field,
            (const char *) field->spec_raw->data,
            field->spec_raw->n,
            e))
        return e->nr;

    ex_set(e, EX_VALUE_ERROR,
        "invalid declaration for `%s` on type `%s`; "
        "expecting a valid type declaration but got `%.*s` instead"
        DOC_T_TYPE,
        field->name->str, field->type->name,
        (int) field->spec_raw->n,
        (const char *) field->spec_raw->data);

    return e->nr;

circular_dep:
    ex_set(e, EX_VALUE_ERROR,
        "invalid declaration for `%s` on type `%s`; "
        "missing `?` after declaration `%.*s`; "
        "circular dependencies must be nillable "
        "at least at one point in the chain"DOC_T_TYPE,
        field->name->str, field->type->name,
        (int) field->spec_raw->n,
        (const char *) field->spec_raw->data);
    return e->nr;
}

ti_val_t * ti_field_dval(ti_field_t * field)
{
    uint16_t spec = field->spec;

    if (field->condition.none)
    {
        ti_val_t * dval = field->condition.none->dval;
        ti_incref(dval);
        return dval;
    }

    if (spec & TI_SPEC_NILLABLE)
         return (ti_val_t *) ti_nil_get();

    spec &= TI_SPEC_MASK_NILLABLE;

    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        return (ti_val_t *) ti_nil_get();
    case TI_SPEC_OBJECT:
        return (ti_val_t *) ti_thing_o_create(
                0,      /* id */
                0,      /* initial size */
                field->type->types->collection);
    case TI_SPEC_RAW:
    case TI_SPEC_STR:
    case TI_SPEC_UTF8:
        return ti_val_empty_str();
    case TI_SPEC_BYTES:
        return ti_val_empty_bin();
    case TI_SPEC_INT:
    case TI_SPEC_UINT:
        return (ti_val_t *) ti_vint_create(0);
    case TI_SPEC_PINT:
        return (ti_val_t *) ti_vint_create(1);
    case TI_SPEC_NINT:
        return (ti_val_t *) ti_vint_create(-1);
    case TI_SPEC_FLOAT:
        return (ti_val_t *) ti_vfloat_create(0.0);
    case TI_SPEC_NUMBER:
        return (ti_val_t *) ti_vint_create(0);
    case TI_SPEC_BOOL:
        return (ti_val_t *) ti_vbool_get(false);
    case TI_SPEC_ARR:
    {
         ti_varr_t * varr = ti_varr_create(0);
         if (varr)
             varr->spec = field->nested_spec;

         return (ti_val_t *) varr;
    }
    case TI_SPEC_SET:
    {
        ti_vset_t * vset = ti_vset_create();
        if (vset)
            vset->spec = field->nested_spec;

        return (ti_val_t *) vset;
    }
    case TI_SPEC_REMATCH:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
    case TI_SPEC_STR_RANGE:
        assert(0);  /* must always have a default value set */
        return NULL;
    }

    return spec < TI_SPEC_ANY
            ? ti_type_dval(ti_types_by_id(field->type->types, spec))
            : ti_enum_dval(ti_enums_by_id(
                    field->type->types->collection->enums,
                    spec & TI_ENUM_ID_MASK));
}

/*
 * If successful, the reference counter for `name` and `spec_raw` will
 * increase.
 */
ti_field_t * ti_field_create(
        ti_name_t * name,
        ti_raw_t * spec_raw,
        ti_type_t * type,
        ex_t * e)
{
    ti_field_t * field = malloc(sizeof(ti_field_t));
    if (!field)
    {
        ex_set_mem(e);
        return NULL;
    }

    field->type = type;
    field->name = name;
    field->spec_raw = spec_raw;
    field->idx = type->fields->n;
    field->condition.none = NULL;

    ti_incref(name);
    ti_incref(spec_raw);

    if (vec_push(&type->fields, field))
    {
        ex_set_mem(e);
        ti_field_destroy(field);
        return NULL;
    }

    if (field__init(field, e))
    {
        assert (e->nr);        ;
        ti_field_destroy(vec_pop(type->fields));
        return NULL;
    }

    assert (e->nr == 0);
    return field;
}

typedef struct
{
    ti_data_t * data;
    ti_name_t * name;
    ti_val_t ** vaddr;
    uint64_t val_idx;
    uint16_t type_id;
    ex_t e;
} field__mod_t;

static int field__mod_nested_cb(ti_thing_t * thing, ti_field_t * field)
{
    if (thing->type_id == field->type->type_id)
    {
        ti_val_t * val = vec_get(thing->items, field->idx);

        switch (val->tp)
        {
        case TI_VAL_NIL:
            return 0;
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
            assert(0);
            return 0;
        case TI_VAL_ARR:
            ((ti_varr_t *) val)->spec = field->nested_spec;
            return 0;
        case TI_VAL_SET:
            ((ti_vset_t *) val)->spec = field->nested_spec;
            return 0;
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
        case TI_VAL_MEMBER:
        case TI_VAL_TEMPLATE:
            assert(0);
            return 0;
        }
    }
    return 0;
}

ti_field_t * ti_field_as_new(ti_field_t * field, ti_raw_t * spec_raw, ex_t * e)
{
    ti_field_t * new_field = malloc(sizeof(ti_field_t));
    if (!new_field)
    {
        ex_set_mem(e);
        return NULL;
    }

    new_field->name = field->name;
    new_field->spec_raw = spec_raw;
    new_field->type = field->type;
    new_field->idx = field->idx;
    new_field->condition.none = NULL;

    ti_incref(new_field->name);
    ti_incref(new_field->spec_raw);

    if (field__init(new_field, e))
    {
        ti_field_destroy(new_field);
        return NULL;  /* error is set */
    }

    return new_field;
}

/*
 * Destroys `with_field` and sets this pointer to NULL
 */
void ti_field_replace(ti_field_t * field, ti_field_t ** with_field)
{
    assert (field->idx == (*with_field)->idx);
    assert (field->type == (*with_field)->type);

    field__remove_dep(field);

    ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
    field->spec = (*with_field)->spec;
    field->nested_spec = (*with_field)->nested_spec;
    field->spec_raw = (*with_field)->spec_raw;

    ti_incref(field->spec_raw);

    ti_field_destroy(*with_field);  /* dependencies are moved */
    *with_field = NULL;
}

int ti_field_mod_force(ti_field_t * field, ti_raw_t * spec_raw, ex_t * e)
{
    ti_raw_t * prev_spec_raw = field->spec_raw;
    ti_condition_via_t prev_condition = field->condition;
    uint16_t prev_spec = field->spec;
    uint16_t prev_nested_spec = field->nested_spec;

    field__remove_dep(field);

    field->spec_raw = spec_raw;

    if (field__init(field, e))
        goto undo;

    if (prev_nested_spec != field->nested_spec)
    {
        (void) imap_walk(
            field->type->types->collection->things,
            (imap_cb) field__mod_nested_cb,
            field);
    }

    ti_incref(spec_raw);
    ti_val_unsafe_drop((ti_val_t *) prev_spec_raw);
    ti_condition_destroy(prev_condition, prev_spec);
    return 0;

undo:
    field->spec_raw = prev_spec_raw;
    field->spec = prev_spec;
    field->nested_spec = prev_nested_spec;
    field->condition = prev_condition;
    (void) field__add_dep(field);

    return e->nr;
}

int ti_field_mod(
        ti_field_t * field,
        ti_raw_t * spec_raw,
        vec_t * vars,
        ex_t * e)
{
    ti_raw_t * prev_spec_raw = field->spec_raw;
    ti_condition_via_t prev_condition = field->condition;
    uint16_t prev_spec = field->spec;
    uint16_t prev_nested_spec = field->nested_spec;

    field__remove_dep(field);

    field->spec_raw = spec_raw;
    if (field__init(field, e))
        goto undo;

    switch (ti__spec_check_mod(
            prev_spec,
            field->spec,
            prev_condition,
            field->condition))
    {
    case TI_SPEC_MOD_SUCCESS:           goto success;
    case TI_SPEC_MOD_ERR:               goto incompatible;
    case TI_SPEC_MOD_NILLABLE_ERR:      goto nillable;
    case TI_SPEC_MOD_NESTED:
        switch (ti__spec_check_mod(
                prev_nested_spec,
                field->nested_spec,
                prev_condition,
                field->condition))
        {
        case TI_SPEC_MOD_SUCCESS:           goto success;
        case TI_SPEC_MOD_ERR:               goto incompatible;
        case TI_SPEC_MOD_NILLABLE_ERR:      goto nillable;
        case TI_SPEC_MOD_NESTED:            goto incompatible;
        }
    }

    assert (0);

nillable:
    ex_set(e, EX_OPERATION_ERROR,
        "cannot apply type declaration `%.*s` to `%s` on type `%s` without a "
        "closure to migrate existing instances; the old declaration "
        "was nillable while the new declaration is not"DOC_MOD_TYPE_MOD,
        (int) spec_raw->n, (const char *) spec_raw->data,
        field->name->str,
        field->type->name);
    goto undo_dep;

incompatible:
    ex_set(e, EX_OPERATION_ERROR,
        "cannot apply type declaration `%.*s` to `%s` on type `%s` without a "
        "closure to migrate existing instances; the old declaration `%.*s` "
        "is not compatible with the new declaration"DOC_MOD_TYPE_MOD,
        (int) spec_raw->n, (const char *) spec_raw->data,
        field->name->str,
        field->type->name,
        (int) prev_spec_raw->n, (const char *) prev_spec_raw->data);

undo_dep:
    field__remove_dep(field);

undo:
    field->spec_raw = prev_spec_raw;
    field->spec = prev_spec;
    field->nested_spec = prev_nested_spec;
    field->condition = prev_condition;
    (void) field__add_dep(field);

    return e->nr;

success:
    if (prev_nested_spec != field->nested_spec)
    {
        /* check for variable to update, val_cache is not required
         * since only things with an id are store in cache
         */
        if (vars && ti_query_vars_walk(
                vars,
                (imap_cb) field__mod_nested_cb,
                field))
        {
            ex_set_mem(e);
            goto undo_dep;
        }

        (void) imap_walk(
            field->type->types->collection->things,
            (imap_cb) field__mod_nested_cb,
            field);
    }
    ti_incref(spec_raw);
    ti_val_unsafe_drop((ti_val_t *) prev_spec_raw);
    ti_condition_destroy(prev_condition, prev_spec);
    return 0;
}

int ti_field_set_name(ti_field_t * field, const char * s, size_t n, ex_t * e)
{
    ti_name_t * name;

    if (!ti_name_is_valid_strn(s, n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "property name must follow the naming rules"DOC_NAMES);
        return e->nr;
    }

    name = ti_names_get(s, n);
    if (!name)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_field_by_name(field->type, name) ||
        ti_method_by_name(field->type, name))
    {
        ex_set(e, EX_VALUE_ERROR,
            "property or method `%s` already exists on type `%s`"DOC_T_TYPE,
            name->str,
            field->type->name);
        goto fail0;
    }

    ti_name_drop(field->name);
    field->name = name;

    return 0;

fail0:
    ti_name_drop(name);
    return e->nr;
}

static void field__del_watch(
        ti_thing_t * thing,
        ti_data_t * data,
        uint64_t event_id)
{
    ti_rpkg_t * rpkg = ti_watch_rpkg(
            thing->id,
            event_id,
            data->data,
            data->n);
    if (!rpkg)
    {
        /*
         * Only log and continue if updating a watcher has failed
         */
        ++ti.counters->watcher_failed;
        log_critical(EX_MEMORY_S);
        return;
    }

    for (vec_each(thing->watchers, ti_watch_t, watch))
    {
        if (ti_stream_is_closed(watch->stream))
            continue;

        if (ti_stream_write_rpkg(watch->stream, rpkg))
        {
            ++ti.counters->watcher_failed;
            log_error(EX_INTERNAL_S);
        }
    }

    ti_rpkg_drop(rpkg);
}

int ti_field_del(ti_field_t * field, uint64_t ev_id)
{
    vec_t * vec = imap_vec(field->type->types->collection->things);
    uint16_t type_id = field->type->type_id;
    ti_data_t * data = field__del_job(field->name->str, field->name->n);

    if (!data || !vec)
        return -1;  /* might leak a few bytes */

    for (vec_each(vec, ti_thing_t, thing))
        ++thing->ref;

    for (vec_each(vec, ti_thing_t, thing))
    {
        if (thing->type_id == type_id)
        {
            if (ti_thing_has_watchers(thing))
                field__del_watch(thing, data, ev_id);

            ti_val_unsafe_drop(vec_swap_remove(thing->items, field->idx));
        }
        ti_val_unsafe_drop((ti_val_t *) thing);
    }

    free(data);
    free(vec);
    ti_field_remove(field);
    return 0;
}

/* remove field from type and destroy; no update of things; */
void ti_field_remove(ti_field_t * field)
{
    ti_field_t * swap;
    if (!field)
        return;

    /* removed dependency if required */
    field__remove_dep(field);

    (void) vec_swap_remove(field->type->fields, field->idx);

    swap = vec_get_or_null(field->type->fields, field->idx);
    if (swap)
        swap->idx = field->idx;

    ti_field_destroy(field);
}

/* just destroy a field object; no remove from type nor update things; */
void ti_field_destroy(ti_field_t * field)
{
    if (!field)
        return;

    ti_name_drop(field->name);
    if (field->spec_raw)
        ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
    ti_condition_destroy(field->condition, field->spec);
    free(field);
}

void ti_field_destroy_dep(ti_field_t * field)
{
    if (!field)
        return;
    field__remove_dep(field);
    ti_field_destroy(field);
}

static inline int field__walk_assign(ti_thing_t * thing, ti_field_t * field)
{
    return thing->type_id != field->nested_spec;
}

static int field__vset_assign(
        ti_field_t * field,
        ti_vset_t ** vset,
        ti_thing_t * parent,
        ex_t * e)
{
    /*
     * This method may be called when field is either `any` or a `set.
     * In case of `any`, we have to make sure the specification will be
     * OBJECT, not ANY.
     */
    uint16_t nested_spec = field->nested_spec == TI_SPEC_ANY
            ? TI_SPEC_OBJECT
            : field->nested_spec;

    if (nested_spec == TI_SPEC_OBJECT ||
        nested_spec == (*vset)->spec ||
        (*vset)->imap->n == 0)
        goto done;

    if (imap_walk((*vset)->imap, (imap_cb) field__walk_assign, field))
    {
        ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` has definition `%.*s` but got a set with "
                "at least one thing of another type",
                field->type->name,
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data);
        return e->nr;
    }

done:
    if (ti_val_make_assignable((ti_val_t **) vset, parent, field->name, e))
        return e->nr;

    (*vset)->spec = nested_spec;
    return 0;
}

static int field__varr_assign(
        ti_field_t * field,
        ti_varr_t ** varr,
        ti_thing_t * parent,
        ex_t * e)
{
    if ((field->nested_spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY ||
        (*varr)->vec->n == 0 ||
        (*varr)->spec == field->nested_spec)
        goto done;

    for (vec_each((*varr)->vec, ti_val_t, val))
    {
        switch (ti_spec_check_nested_val(field->nested_spec, val))
        {
        case TI_SPEC_RVAL_SUCCESS:
            continue;
        case TI_SPEC_RVAL_TYPE_ERROR:
            ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with items that matches "
                "definition `%.*s`",
                field->type->name,
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data);
            return e->nr;
        case TI_SPEC_RVAL_UTF8_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with UTF8 string values",
                field->type->name,
                field->name->str);
            return e->nr;
        case TI_SPEC_RVAL_UINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with integer values "
                "greater than or equal to 0",
                field->type->name,
                field->name->str);
            return e->nr;
        case TI_SPEC_RVAL_PINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with positive integer values",
                field->type->name,
                field->name->str);
            return e->nr;
        case TI_SPEC_RVAL_NINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with negative integer values",
                field->type->name,
                field->name->str);
            return e->nr;
        }
    }

done:
    if (ti_val_make_assignable((ti_val_t **) varr, parent, field->name, e))
        return e->nr;

    (*varr)->spec = field->nested_spec;
    return 0;
}

static _Bool field__maps_arr_to_arr(ti_field_t * field, ti_varr_t * varr)
{
    if ((field->nested_spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY ||
        varr->vec->n == 0 ||
        varr->spec == field->nested_spec)
        return true;

    for (vec_each(varr->vec, ti_val_t, val))
        if (!ti_spec_maps_to_nested_val(field->nested_spec, val))
            return false;

    return true;
}

static _Bool field__maps_set_to_arr(ti_field_t * field)
{
    /*
     * A set can only map to an array if, and only if the array accepts a
     * custom type or `any` type, or thing type. Resolves issue #65.
     */
    return ((field->nested_spec & TI_SPEC_MASK_NILLABLE) <= TI_SPEC_OBJECT);
}

/*
 * Returns 0 if the given value is valid for this field
 */
int ti_field_make_assignable(
        ti_field_t * field,
        ti_val_t ** val,
        ti_thing_t * parent,  /* may be NULL */
        ex_t * e)
{
    uint16_t spec = field->spec;

    if ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(*val))
        return 0;

    spec &= TI_SPEC_MASK_NILLABLE;

    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        switch ((ti_val_enum) (*val)->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
            break;
        case TI_VAL_ARR:
            return field__varr_assign(field, (ti_varr_t **) val, parent, e);
        case TI_VAL_SET:
            return field__vset_assign(field, (ti_vset_t **) val, parent, e);
        case TI_VAL_CLOSURE:
            return ti_val_is_closure(*val);
        case TI_VAL_ERROR:
        case TI_VAL_MEMBER:
        case TI_VAL_TEMPLATE:
            break;
        }
        return 0;
    case TI_SPEC_OBJECT:
        if (ti_val_is_thing(*val))
            return 0;
        goto type_error;
    case TI_SPEC_RAW:
        if (ti_val_is_raw(*val))
            return 0;
        goto type_error;
    case TI_SPEC_STR:
        if (ti_val_is_str(*val))
            return 0;
        goto type_error;
    case TI_SPEC_UTF8:
        if (!ti_val_is_str(*val))
            goto type_error;
        if (!strx_is_utf8n(
                (const char *) ((ti_raw_t *) *val)->data,
                ((ti_raw_t *) *val)->n))
            goto utf8_error;
        return 0;
    case TI_SPEC_BYTES:
        if (ti_val_is_bytes(*val))
            return 0;
        goto type_error;
    case TI_SPEC_INT:
        if (ti_val_is_int(*val))
            return 0;
        goto type_error;
    case TI_SPEC_UINT:
        if (!ti_val_is_int(*val))
            goto type_error;
        if (VINT(*val) < 0)
            goto uint_error;
        return 0;
    case TI_SPEC_PINT:
        if (!ti_val_is_int(*val))
            goto type_error;
        if (VINT(*val) <= 0)
            goto pint_error;
        return 0;
    case TI_SPEC_NINT:
        if (!ti_val_is_int(*val))
            goto type_error;
        if (VINT(*val) >= 0)
            goto nint_error;
        return 0;
    case TI_SPEC_FLOAT:
        if (ti_val_is_float(*val))
            return 0;
        goto type_error;
    case TI_SPEC_NUMBER:
        if (ti_val_is_number(*val))
            return 0;
        goto type_error;
    case TI_SPEC_BOOL:
        if (ti_val_is_bool(*val))
            return 0;
        goto type_error;
    case TI_SPEC_ARR:
        if (ti_val_is_array(*val))
            return field__varr_assign(field, (ti_varr_t **) val, parent, e);
        goto type_error;
    case TI_SPEC_SET:
        if (ti_val_is_set(*val))
            return field__vset_assign(field, (ti_vset_t **) val, parent, e);
        goto type_error;

    case TI_SPEC_REMATCH:
        if (!ti_val_is_str(*val))
            goto type_error;
        if (!ti_regex_test(field->condition.re->regex, (ti_raw_t *) *val))
            goto re_error;
        return 0;
    case TI_SPEC_INT_RANGE:
        if (!ti_val_is_int(*val))
            goto type_error;
        if (VINT(*val) < field->condition.irange->mi ||
            VINT(*val) > field->condition.irange->ma)
            goto irange_error;
        return 0;
    case TI_SPEC_FLOAT_RANGE:
        if (!ti_val_is_float(*val))
            goto type_error;
        if (VFLOAT(*val) < field->condition.drange->mi ||
            VFLOAT(*val) > field->condition.drange->ma)
            goto drange_error;
        if (isnan(VFLOAT(*val)))
            goto nan_error;
        return 0;
    case TI_SPEC_STR_RANGE:
        if (!ti_val_is_str(*val))
            goto type_error;
        if (((ti_raw_t *) *val)->n < field->condition.srange->mi ||
            ((ti_raw_t *) *val)->n > field->condition.srange->ma)
            goto srange_error;
        return 0;
    }

    if (spec >= TI_ENUM_ID_FLAG)
    {
        if (ti_spec_enum_eq_to_val(spec, *val))
            return 0;

        goto type_error;
    }

    assert (spec < TI_SPEC_ANY);

    /*
     * Just compare the specification with the type since the nillable mask is
     * removed the specification
     */
    if (ti_val_is_thing(*val) && ((ti_thing_t *) *val)->type_id == spec)
        return 0;

    goto type_error;

irange_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a float value between "
            "%"PRId64" and %"PRId64" (both inclusive)",
            field->type->name,
            field->name->str,
            field->condition.irange->mi,
            field->condition.irange->ma);
    return e->nr;

drange_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a float value between "
            "%f and %f (both inclusive)",
            field->type->name,
            field->name->str,
            field->condition.drange->mi,
            field->condition.drange->ma);
    return e->nr;


nan_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a float value between "
            "%f and %f but got nan (not a number)",
            field->type->name,
            field->name->str,
            field->condition.drange->mi,
            field->condition.drange->ma);
    return e->nr;

srange_error:
    if (field->condition.srange->mi == field->condition.srange->ma)
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires a string with a length "
                "of %zu character%s",
                field->type->name,
                field->name->str,
                field->condition.srange->mi,
                field->condition.srange->mi == 1 ? "": "s");
    else
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires a string with a length "
                "between %zu and %zu (both inclusive) characters",
                field->type->name,
                field->name->str,
                field->condition.srange->mi,
                field->condition.srange->ma);
    return e->nr;

re_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` has a requirement to match pattern %.*s",
            field->type->name,
            field->name->str,
            (int) field->condition.re->regex->pattern->n,
            (const char *) field->condition.re->regex->pattern->data);
    return e->nr;

uint_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` only accepts integer values "
            "greater than or equal to 0",
            field->type->name,
            field->name->str);
    return e->nr;

pint_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` only accepts positive integer values",
            field->type->name,
            field->name->str);
    return e->nr;

nint_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` only accepts negative integer values",
            field->type->name,
            field->name->str);
    return e->nr;

utf8_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` only accepts valid UTF8 data",
            field->type->name,
            field->name->str);
    return e->nr;

type_error:
    ex_set(e, EX_TYPE_ERROR,
            "mismatch in type `%s`; "
            "type `%s` is invalid for property `%s` with definition `%.*s`",
            field->type->name,
            ti_val_str(*val),
            field->name->str,
            (int) field->spec_raw->n,
            (const char *) field->spec_raw->data);
    return e->nr;
}

_Bool ti_field_maps_to_val(ti_field_t * field, ti_val_t * val)
{
    uint16_t spec = field->spec;

    if ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(val))
        return true;

    spec &= TI_SPEC_MASK_NILLABLE;

    if (spec >= TI_ENUM_ID_FLAG)
        return ti_spec_enum_eq_to_val(spec, val);

    if (ti_val_is_member(val))
        val = VMEMBER(val);

    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        return true;
    case TI_SPEC_OBJECT:
        return ti_val_is_thing(val);
    case TI_SPEC_RAW:
        return ti_val_is_raw(val);
    case TI_SPEC_STR:
        return ti_val_is_str(val);
    case TI_SPEC_UTF8:
        return ti_val_is_str(val) && strx_is_utf8n(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_SPEC_BYTES:
        return ti_val_is_bytes(val);
    case TI_SPEC_INT:
        return ti_val_is_int(val);
    case TI_SPEC_UINT:
        return ti_val_is_int(val) && VINT(val) >= 0;
    case TI_SPEC_PINT:
        return ti_val_is_int(val) && VINT(val) > 0;
    case TI_SPEC_NINT:
        return ti_val_is_int(val) && VINT(val) < 0;
    case TI_SPEC_FLOAT:
        return ti_val_is_float(val);
    case TI_SPEC_NUMBER:
        return ti_val_is_number(val);
    case TI_SPEC_BOOL:
        return ti_val_is_bool(val);
    case TI_SPEC_ARR:
        /* we can map a set to an array */
        return ((
            ti_val_is_array(val) &&
            field__maps_arr_to_arr(field, (ti_varr_t *) val)
        ) || (
            ti_val_is_set(val) && field__maps_set_to_arr(field)
        ));
    case TI_SPEC_SET:
        return ti_val_is_set(val);
    case TI_SPEC_REMATCH:
        return (ti_val_is_str(val) &&
                ti_regex_test(field->condition.re->regex, (ti_raw_t *) val));
    case TI_SPEC_INT_RANGE:
        return (ti_val_is_int(val) &&
                VINT(val) >= field->condition.irange->mi &&
                VINT(val) <= field->condition.irange->ma);
    case TI_SPEC_FLOAT_RANGE:
        return (ti_val_is_float(val) &&
                VFLOAT(val) >= field->condition.drange->mi &&
                VFLOAT(val) <= field->condition.drange->ma &&
                !isnan(VFLOAT(val)));
    case TI_SPEC_STR_RANGE:
        return (ti_val_is_str(val) &&
                ((ti_raw_t *) val)->n >= field->condition.srange->mi &&
                ((ti_raw_t *) val)->n <= field->condition.srange->ma);
    }

    /* any *thing* can be mapped */
    return ti_val_is_thing(val);
}

static _Bool field__maps_to_nested(ti_field_t * t_field, ti_field_t * f_field)
{
    uint16_t t_spec, f_spec;

    /* both the t_field and f_field are either a set or array */

    assert ((f_field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ARR ||
            (f_field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET);

    assert ((t_field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ARR ||
            (t_field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET);

    if ((t_field->nested_spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY)
        return true;

    if (    (~t_field->nested_spec & TI_SPEC_NILLABLE) &&
            (f_field->nested_spec & TI_SPEC_NILLABLE))
        return false;

    t_spec = t_field->nested_spec & TI_SPEC_MASK_NILLABLE;
    f_spec = f_field->nested_spec & TI_SPEC_MASK_NILLABLE;

    if (t_spec >= TI_ENUM_ID_FLAG)
        return t_spec == f_spec;

    if (f_spec >= TI_ENUM_ID_FLAG)
    {
        ti_enum_t * enum_ = ti_enums_by_id(
                f_field->type->types->collection->enums,
                f_spec & TI_ENUM_ID_MASK);
        f_spec = ti_enum_spec(enum_);
    }

    if (t_spec == f_spec)
        return true;

    switch ((ti_spec_enum_t) t_spec)
    {
    case TI_SPEC_ANY:
        return true;       /* already checked */
    case TI_SPEC_OBJECT:
        return f_spec < TI_SPEC_ANY;
    case TI_SPEC_RAW:
        return (f_spec == TI_SPEC_STR ||
                f_spec == TI_SPEC_UTF8 ||
                f_spec == TI_SPEC_BYTES);
    case TI_SPEC_INT:
        return (f_spec == TI_SPEC_UINT ||
                f_spec == TI_SPEC_PINT ||
                f_spec == TI_SPEC_NINT);
    case TI_SPEC_UINT:
        return f_spec == TI_SPEC_PINT;
    case TI_SPEC_NUMBER:
        return (f_spec == TI_SPEC_INT ||
                f_spec == TI_SPEC_UINT ||
                f_spec == TI_SPEC_PINT ||
                f_spec == TI_SPEC_NINT ||
                f_spec == TI_SPEC_FLOAT);
    case TI_SPEC_STR:
        return f_spec == TI_SPEC_UTF8;
    case TI_SPEC_UTF8:
    case TI_SPEC_BYTES:
    case TI_SPEC_PINT:
    case TI_SPEC_NINT:
    case TI_SPEC_FLOAT:
    case TI_SPEC_BOOL:
    case TI_SPEC_ARR:
    case TI_SPEC_SET:
    case TI_SPEC_REMATCH:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
    case TI_SPEC_STR_RANGE:
        return false;
    }

    assert (t_spec < TI_SPEC_ANY);  /* enumerators are already checked */

    return f_spec < TI_SPEC_ANY || f_spec == TI_SPEC_OBJECT;
}

_Bool field__maps_with_condition(ti_field_t * t_field, ti_field_t * f_field)
{
    uint16_t spec = t_field->spec & TI_SPEC_MASK_NILLABLE;
    assert (t_field->condition.none);
    assert (f_field->condition.none);

    switch((ti_spec_enum_t) spec)
    {
    case TI_SPEC_REMATCH:
        return ti_regex_eq(
                t_field->condition.re->regex,
                f_field->condition.re->regex);
    case TI_SPEC_STR_RANGE:
        return (
            t_field->condition.srange->mi <= f_field->condition.srange->mi &&
            t_field->condition.srange->ma >= f_field->condition.srange->ma);
    case TI_SPEC_INT_RANGE:
        return (
            t_field->condition.irange->mi <= f_field->condition.irange->mi &&
            t_field->condition.irange->ma >= f_field->condition.irange->ma);
    case TI_SPEC_FLOAT_RANGE:
        return (
            t_field->condition.drange->mi <= f_field->condition.drange->mi &&
            t_field->condition.drange->ma >= f_field->condition.drange->ma);
    default:
        assert(0);
        return false;
    }
}

_Bool ti_field_maps_to_field(ti_field_t * t_field, ti_field_t * f_field)
{
    uint16_t t_spec, f_spec;
    assert (t_field->name == f_field->name);

    /* return 0 when `to` accepts `any` */
    if ((t_field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY)
        return true;

    /* if `to` does not accept `nil`, and from does, this is an error */
    if (    (~t_field->spec & TI_SPEC_NILLABLE) &&
            (f_field->spec & TI_SPEC_NILLABLE))
        return false;

    t_spec = t_field->spec & TI_SPEC_MASK_NILLABLE;
    f_spec = f_field->spec & TI_SPEC_MASK_NILLABLE;

    if (t_spec >= TI_ENUM_ID_FLAG)
        return t_spec == f_spec;

    if (f_spec >= TI_ENUM_ID_FLAG)
    {
        ti_enum_t * enum_ = ti_enums_by_id(
                f_field->type->types->collection->enums,
                f_spec & TI_ENUM_ID_MASK);
        f_spec = ti_enum_spec(enum_);
    }

    switch ((ti_spec_enum_t) t_spec)
    {
    case TI_SPEC_ANY:
        return true;       /* already checked */
    case TI_SPEC_OBJECT:
        return (f_spec == TI_SPEC_OBJECT ||
                f_spec < TI_SPEC_ANY);
    case TI_SPEC_RAW:
        return (f_spec == TI_SPEC_RAW ||
                f_spec == TI_SPEC_STR ||
                f_spec == TI_SPEC_UTF8 ||
                f_spec == TI_SPEC_BYTES ||
                f_spec == TI_SPEC_REMATCH ||
                f_spec == TI_SPEC_STR_RANGE);
    case TI_SPEC_STR:
        return (f_spec == TI_SPEC_STR ||
                f_spec == TI_SPEC_UTF8 ||
                f_spec == TI_SPEC_REMATCH ||
                f_spec == TI_SPEC_STR_RANGE);
    case TI_SPEC_UTF8:
    case TI_SPEC_BYTES:
        return f_spec == t_spec;
    case TI_SPEC_INT:
        return (f_spec == TI_SPEC_INT ||
                f_spec == TI_SPEC_UINT ||
                f_spec == TI_SPEC_PINT ||
                f_spec == TI_SPEC_NINT ||
                f_spec == TI_SPEC_INT_RANGE);
    case TI_SPEC_UINT:
        return (
            f_spec == TI_SPEC_UINT ||
            f_spec == TI_SPEC_PINT ||
            (f_spec == TI_SPEC_INT_RANGE && f_field->condition.irange->mi >= 0)
        );
    case TI_SPEC_PINT:
        return (
            f_spec == TI_SPEC_PINT ||
            (f_spec == TI_SPEC_INT_RANGE && f_field->condition.irange->mi > 0)
        );
    case TI_SPEC_NINT:
        return (
            f_spec == TI_SPEC_NINT ||
            (f_spec == TI_SPEC_INT_RANGE && f_field->condition.irange->ma < 0)
        );
    case TI_SPEC_FLOAT:
        return (f_spec == TI_SPEC_FLOAT ||
                f_spec == TI_SPEC_FLOAT_RANGE);
    case TI_SPEC_NUMBER:
        return (f_spec == TI_SPEC_NUMBER ||
                f_spec == TI_SPEC_FLOAT ||
                f_spec == TI_SPEC_INT ||
                f_spec == TI_SPEC_UINT ||
                f_spec == TI_SPEC_PINT ||
                f_spec == TI_SPEC_NINT ||
                f_spec == TI_SPEC_INT_RANGE ||
                f_spec == TI_SPEC_FLOAT_RANGE);
    case TI_SPEC_BOOL:
        return f_spec == t_spec;
    case TI_SPEC_ARR:
        return (
            (f_spec == TI_SPEC_ARR || f_spec == TI_SPEC_SET) &&
            field__maps_to_nested(t_field, f_field)
        );
    case TI_SPEC_SET:
        return f_spec == TI_SPEC_SET;
    case TI_SPEC_REMATCH:
        return f_spec == TI_SPEC_REMATCH && ti_regex_eq(
                        t_field->condition.re->regex,
                        f_field->condition.re->regex);
    case TI_SPEC_INT_RANGE:
        return (
            f_spec == TI_SPEC_INT_RANGE &&
            f_field->condition.irange->mi >= t_field->condition.irange->mi &&
            f_field->condition.irange->ma <= t_field->condition.irange->ma);
    case TI_SPEC_FLOAT_RANGE:
        return (
            f_spec == TI_SPEC_FLOAT_RANGE &&
            f_field->condition.drange->mi >= t_field->condition.drange->mi &&
            f_field->condition.drange->ma <= t_field->condition.drange->ma);
    case TI_SPEC_STR_RANGE:
        return (
            f_spec == TI_SPEC_STR_RANGE &&
            f_field->condition.srange->mi >= t_field->condition.srange->mi &&
            f_field->condition.srange->ma <= t_field->condition.srange->ma);
    }

    assert (t_spec < TI_SPEC_ANY);

    return f_spec < TI_SPEC_ANY || f_spec == TI_SPEC_OBJECT;
}

ti_field_t * ti_field_by_name(ti_type_t * type, ti_name_t * name)
{
    for (vec_each(type->fields, ti_field_t, field))
        if (field->name == name)
            return field;
    return NULL;
}

ti_field_t * ti_field_by_strn_e(
        ti_type_t * type,
        const char * str,
        size_t n,
        ex_t * e)
{
    ti_name_t * name = ti_names_weak_get_strn(str, n);
    if (name)
        for (vec_each(type->fields, ti_field_t, field))
            if (field->name == name)
                return field;

    if (ti_name_is_valid_strn(str, n))
        ex_set(e, EX_LOOKUP_ERROR, "type `%s` has no property `%.*s`",
                type->name,
                (int) n, str);
    else
        ex_set(e, EX_VALUE_ERROR,
                "property name must follow the naming rules"DOC_NAMES);

    return NULL;
}

typedef struct
{
    ti_data_t * data;
    ti_name_t * name;
    ti_val_t ** vaddr;
    uint64_t event_id;
    uint16_t type_id;
    ex_t e;
} field__add_t;

static int field__add(ti_thing_t * thing, field__add_t * w)
{
    if (thing->type_id != w->type_id)
        return 0;

    /* closure is already unbound, so only a memory exception can occur */
    if (ti_val_make_assignable(w->vaddr, thing, w->name, &w->e) ||
        vec_push(&thing->items, *w->vaddr))
        return 1;

    ti_incref(*w->vaddr);

    if (ti_thing_has_watchers(thing))
    {
        ti_rpkg_t * rpkg = ti_watch_rpkg(
                thing->id,
                w->event_id,
                w->data->data,
                w->data->n);

        if (rpkg)
        {
            for (vec_each(thing->watchers, ti_watch_t, watch))
            {
                if (ti_stream_is_closed(watch->stream))
                    continue;

                if (ti_stream_write_rpkg(watch->stream, rpkg))
                {
                    ++ti.counters->watcher_failed;
                    log_error(EX_INTERNAL_S);
                }
            }
            ti_rpkg_drop(rpkg);
        }
        else
        {
            /*
             * Only log and continue if updating a watcher has failed
             */
            ++ti.counters->watcher_failed;
            log_critical(EX_MEMORY_S);
        }
    }
    return 0;
}

/*
 * Use only when adding a new field, to update the existing things and
 * notify optional watchers;
 *
 * warn: ti_val_gen_ids() must have used on the given value before using this
 *       function
 * error: can only fail in case of a memory allocation error
 */
int ti_field_init_things(ti_field_t * field, ti_val_t ** vaddr, uint64_t ev_id)
{
    assert (field == vec_last(field->type->fields));
    int rc;
    field__add_t addjob = {
            .data = field___set_job(field->name, *vaddr),
            .name = field->name,
            .vaddr = vaddr,
            .event_id = ev_id,
            .type_id = field->type->type_id,
            .e = {0}
    };

    if (!addjob.data)
        return -1;

    rc = imap_walk(
            field->type->types->collection->things,
            (imap_cb) field__add,
            &addjob);

    free(addjob.data);
    return rc;
}

