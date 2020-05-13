/*
 * ti/field.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <ti/data.h>
#include <ti/field.h>
#include <ti/names.h>
#include <ti/spec.h>
#include <ti/enum.h>
#include <ti/spec.inline.h>
#include <ti/thing.inline.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vint.h>
#include <ti/vset.h>
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
                DOC_SPEC,
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
                "type declarations must not be empty"DOC_SPEC,
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

skip_nesting:

    assert (n);

    if (*str == '[')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `[`; nested array declarations are not allowed"
            DOC_SPEC, field->name->str, field->type->name);
        return e->nr;
    }

    if (*str == '{')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `{`; nested set declarations are not allowed"
            DOC_SPEC, field->name->str, field->type->name);
        return e->nr;
    }

    if (field__cmp(str, n, "any"))
    {
        *spec |= TI_SPEC_ANY;
    }
    else if (field__cmp(str, n, TI_VAL_THING_S))
    {
        *spec |= TI_SPEC_OBJECT;
    }
    else if (field__cmp(str, n, "raw"))
    {
        *spec |= TI_SPEC_RAW;
    }
    else if (field__cmp(str, n, TI_VAL_STR_S))
    {
        *spec |= TI_SPEC_STR;
    }
    else if (field__cmp(str, n, "utf8"))
    {
        *spec |= TI_SPEC_UTF8;
    }
    else if (field__cmp(str, n, TI_VAL_BYTES_S))
    {
        *spec |= TI_SPEC_BYTES;
    }
    else if (field__cmp(str, n, TI_VAL_INT_S))
    {
        *spec |= TI_SPEC_INT;
    }
    else if (field__cmp(str, n, "uint"))
    {
        *spec |= TI_SPEC_UINT;
    }
    else if (field__cmp(str, n, "pint"))
    {
        *spec |= TI_SPEC_PINT;
    }
    else if (field__cmp(str, n, "nint"))
    {
        *spec |= TI_SPEC_NINT;
    }
    else if (field__cmp(str, n, TI_VAL_FLOAT_S))
    {
        *spec |= TI_SPEC_FLOAT;
    }
    else if (field__cmp(str, n, "number"))
    {
        *spec |= TI_SPEC_NUMBER;
    }
    else if (field__cmp(str, n, TI_VAL_BOOL_S))
    {
        *spec |= TI_SPEC_BOOL;
    }
    else if (field__cmp(str, n, field->type->name))
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
                            "unknown type `%.*s` in declaration"DOC_SPEC,
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

            if (&field->spec == spec && (~field->spec & TI_SPEC_NILLABLE) &&
                field__dep_check(dep, field->type))
                goto circular_dep;
        }

        if (vec_push(&field->type->dependencies, dep))
        {
            ex_set_mem(e);
            return e->nr;
        }

        ++dep->refcount;
        return 0;
    }

    if ((field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET &&
        field->nested_spec > TI_SPEC_OBJECT)
    {
        if (field->nested_spec & TI_SPEC_NILLABLE)
            ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type `"TI_VAL_SET_S"` cannot contain type `"TI_VAL_NIL_S"`",
                DOC_SPEC,
                field->name->str, field->type->name);
        else
            ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type `"TI_VAL_SET_S"` cannot contain type `%s`"
                DOC_SPEC,
                field->name->str, field->type->name,
                ti__spec_approx_type_str(field->nested_spec));
        return e->nr;
    }

    return 0;

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
        DOC_SPEC,
        field->name->str, field->type->name,
        (int) field->spec_raw->n,
        (const char *) field->spec_raw->data);

    return e->nr;

circular_dep:
    ex_set(e, EX_VALUE_ERROR,
        "invalid declaration for `%s` on type `%s`; "
        "missing `?` after declaration `%.*s`; "
        "circular dependencies must be nillable "
        "at least at one point in the chain"DOC_SPEC,
        field->name->str, field->type->name,
        (int) field->spec_raw->n,
        (const char *) field->spec_raw->data);
    return e->nr;
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

int ti_field_mod(ti_field_t * field, ti_raw_t * spec_raw, size_t n, ex_t * e)
{
    ti_raw_t * prev_spec_raw = field->spec_raw;
    uint16_t prev_spec = field->spec;
    uint16_t prev_nested_spec = field->nested_spec;

    field__remove_dep(field);

    field->spec_raw = spec_raw;
    if (field__init(field, e))
        goto undo;

    if (!n)
        goto success;

    switch (ti__spec_check_mod(prev_spec, field->spec))
    {
    case TI_SPEC_MOD_SUCCESS:           goto success;
    case TI_SPEC_MOD_ERR:               goto incompatible;
    case TI_SPEC_MOD_NILLABLE_ERR:      goto nillable;
    case TI_SPEC_MOD_NESTED:
        switch (ti__spec_check_mod(prev_nested_spec, field->nested_spec))
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
        "cannot apply type declaration `%.*s` to `%s` on type `%s`; "
        "type `%s` has %zu active instance%s and the old declaration "
        "was nillable while the new declaration is not"DOC_MOD_TYPE_MOD,
        (int) spec_raw->n, (const char *) spec_raw->data,
        field->name->str,
        field->type->name,
        field->type->name,
        n, n == 1 ? "" : "s");
    goto undo_dep;

incompatible:
    ex_set(e, EX_OPERATION_ERROR,
        "cannot apply type declaration `%.*s` to `%s` on type `%s`; "
        "type `%s` has %zu active instance%s and the old declaration `%.*s` "
        "is not compatible with the new declaration"DOC_MOD_TYPE_MOD,
        (int) spec_raw->n, (const char *) spec_raw->data,
        field->name->str,
        field->type->name,
        field->type->name,
        n, n == 1 ? "" : "s",
        (int) prev_spec_raw->n, (const char *) prev_spec_raw->data);

undo_dep:
    field__remove_dep(field);

undo:
    field->spec_raw = prev_spec_raw;
    field->spec = prev_spec;
    field->nested_spec = prev_nested_spec;
    (void) field__add_dep(field);

    return e->nr;

success:
    ti_incref(spec_raw);
    ti_val_drop((ti_val_t *) prev_spec_raw);
    return 0;
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

            ti_val_drop(vec_swap_remove(thing->items, field->idx));
        }
        ti_val_drop((ti_val_t *) thing);
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
    ti_val_drop((ti_val_t *) field->spec_raw);

    free(field);
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
    if (field->nested_spec == TI_SPEC_ANY ||
        field->nested_spec == (*vset)->spec ||
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
    if (ti_val_make_assignable((ti_val_t **) vset, parent, field->name, e) == 0)
        (*vset)->spec = field->nested_spec;
    return e->nr;
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
        switch (ti_spec_check_val(field->nested_spec, val))
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
    if (ti_val_make_assignable((ti_val_t **) varr, parent, field->name, e) == 0)
        (*varr)->spec = field->nested_spec;
    return e->nr;
}

static _Bool field__maps_to_varr(ti_field_t * field, ti_varr_t * varr)
{
    if ((field->nested_spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY ||
        varr->vec->n == 0 ||
        varr->spec == field->nested_spec)
        return true;

    for (vec_each(varr->vec, ti_val_t, val))
        if (!ti_spec_maps_to_val(field->nested_spec, val))
            return false;

    return true;
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
    switch (ti_spec_check_val(field->spec, *val))
    {
    case TI_SPEC_RVAL_SUCCESS:
        return ti_val_is_array(*val)
                ? field__varr_assign(field, (ti_varr_t **) val, parent, e)
                : ti_val_is_set(*val)
                ? field__vset_assign(field, (ti_vset_t **) val, parent, e)
                : ti_val_is_closure(*val)
                ? ti_closure_unbound((ti_closure_t * ) *val, e)
                : 0;
    case TI_SPEC_RVAL_TYPE_ERROR:
        ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s`; "
                "type `%s` is invalid for property `%s` with definition `%.*s`",
                field->type->name,
                ti_val_str(*val),
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data);
        break;
    case TI_SPEC_RVAL_UTF8_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` only accepts valid UTF8 data",
                field->type->name,
                field->name->str);
        break;
    case TI_SPEC_RVAL_UINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` only accepts integer values "
                "greater than or equal to 0",
                field->type->name,
                field->name->str);
        break;
    case TI_SPEC_RVAL_PINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` only accepts positive integer values",
                field->type->name,
                field->name->str);
        break;
    case TI_SPEC_RVAL_NINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` only accepts negative integer values",
                field->type->name,
                field->name->str);
        break;
    }
    return e->nr;
}

_Bool ti_field_maps_to_val(ti_field_t * field, ti_val_t * val)
{
    return ti_spec_maps_to_val(field->spec, val)
            ? ti_val_is_array(val)
            ? field__maps_to_varr(field, (ti_varr_t *) val)
            : true
            : false;
}

static _Bool field__maps_to_spec(uint16_t t_spec, uint16_t f_spec)
{
    switch ((ti_spec_enum_t) t_spec)
    {
    case TI_SPEC_ANY:
        return false;       /* already checked */
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
        return false;
    }

    assert (t_spec < TI_SPEC_ANY);  /* enumerators are already checked */

    return f_spec < TI_SPEC_ANY;
}

static _Bool field__maps_to_nested(ti_field_t * t_field, ti_field_t * f_field)
{
    uint16_t t_spec, f_spec;

    assert ((t_field->spec & TI_SPEC_MASK_NILLABLE) ==
            (f_field->spec & TI_SPEC_MASK_NILLABLE));
    assert (t_field->nested_spec != TI_SPEC_ANY);

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
                f_spec);
        f_spec = ti_enum_spec(enum_);
    }

    if (t_spec == f_spec)
        return true;

    return field__maps_to_spec(t_spec, f_spec);
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
                f_spec);
        f_spec = ti_enum_spec(enum_);
    }

    /* return `true` when both specifications are equal, and nested accepts
     * anything which is default for all other than `arr` and `set` */
    return t_spec == f_spec
            ? (t_field->nested_spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY
            ? true
            : field__maps_to_nested(t_field, f_field)
            : field__maps_to_spec(t_spec, f_spec);
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
    ti_name_t * name = ti_names_weak_get(str, n);
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
