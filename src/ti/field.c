/*
 * ti/field.c
 */
#include <assert.h>
#include <doc.h>
#include <math.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/closure.h>
#include <ti/condition.h>
#include <ti/data.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/field.h>
#include <ti/gc.h>
#include <ti/method.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/query.h>
#include <ti/room.h>
#include <ti/spec.h>
#include <ti/spec.inline.h>
#include <ti/thing.inline.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vbool.h>
#include <ti/verror.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/vtask.h>
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
    {
        type->selfref--;
        return;  /* self references are not counted within dependencies */
    }
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
        /* self references are not counted within dependencies */
        return ++type->selfref, 0;

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

static ti_val_t * field__dval_nil(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_nil_get();
}

static ti_val_t * field__dval_arr(ti_field_t * UNUSED(field))
{
     return (ti_val_t *) ti_varr_create(0);
}

static ti_val_t * field__dval_set(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_vset_create();;
}

static ti_val_t * field__dval_thing(ti_field_t * field)
{
    return (ti_val_t *) ti_thing_o_create(
            0,      /* id */
            0,      /* initial size */
            field->type->types->collection);
}

static ti_val_t * field__dval_restricted(ti_field_t * field)
{
     ti_thing_t * thing = ti_thing_o_create(
            0,      /* id */
            0,      /* initial size */
            field->type->types->collection);
     if (thing)
         thing->via.spec = field->nested_spec;
     return (ti_val_t *) thing;
}

static ti_val_t * field__dval_str(ti_field_t * UNUSED(field))
{
    return ti_val_empty_str();
}

static ti_val_t * field__dval_bin(ti_field_t * UNUSED(field))
{
    return ti_val_empty_bin();
}

static ti_val_t * field__dval_int(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_vint_create(0);
}

static ti_val_t * field__dval_pint(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_vint_create(1);
}

static ti_val_t * field__dval_nint(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_vint_create(-1);
}

static ti_val_t * field__dval_float(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_vfloat_create(0.0);
}

static ti_val_t * field__dval_bool(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_vbool_get(false);
}

static ti_val_t * field__dval_regex(ti_field_t * UNUSED(field))
{
    return ti_val_default_re();
}

static ti_val_t * field__dval_closure(ti_field_t * UNUSED(field))
{
    return ti_val_default_closure();
}

static ti_val_t * field__dval_error(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_verror_from_code(-100);
}

static ti_val_t * field__dval_room(ti_field_t * field)
{
    return (ti_val_t *) ti_room_create(0, field->type->types->collection);
}

static ti_val_t * field__dval_task(ti_field_t * UNUSED(field))
{
    return (ti_val_t *) ti_vtask_nil();
}

static ti_val_t * field__dval_datetime(ti_field_t * field)
{
    return (ti_val_t *) ti_datetime_from_u64(
            util_now_usec(),
            field->type->types->collection->tz);
}

static ti_val_t * field__dval_timeval(ti_field_t * field)
{
    return (ti_val_t *) ti_timeval_from_u64(
            util_now_usec(),
            field->type->types->collection->tz);
}

static ti_val_t * field__dval_type(ti_field_t * field)
{
    return ti_type_dval(ti_types_by_id(field->type->types, field->spec));
}

static ti_val_t * field__dval_enum(ti_field_t * field)
{
    return ti_enum_dval(ti_enums_by_id(
            field->type->types->collection->enums,
            field->spec & TI_ENUM_ID_MASK));
}

static inline void field__set_cb(ti_field_t * field, ti_field_dval_cb cb)
{
    if (!field->dval_cb)
        field->dval_cb = cb;
}

/*
 * The following `enum` and `asso_values` are generated using `gperf` and
 * the utility `pcregrep` to generate the input.
 *
 * Command:
 *
 *    pcregrep -o1 '\.name\=\"([\w\{\}\[\]]+)' field.c | gperf -E -k '*,1,$' -m 200
 */
enum
{
    TOTAL_KEYWORDS = 25,
    MIN_WORD_LENGTH = 2,
    MAX_WORD_LENGTH = 8,
    MIN_HASH_VALUE = 2,
    MAX_HASH_VALUE = 29
};

static inline unsigned int field__hash(
        register const char * s,
        register size_t n)
{
    static unsigned short asso_values[] =
    {
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 13, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30,  8, 30,  8, 30, 30, 30,  9,  6,  0,
         0,  4,  3,  1,  9,  0, 30,  4,  0,  4,
         1,  0,  9, 30,  0,  8,  0,  0,  0,  0,
         1,  0, 30,  0, 30,  0, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30
    };

    register unsigned int hval = n;

    switch (hval)
    {
        default:
            hval += asso_values[(unsigned char)s[7]];
            /*fall through*/
        case 7:
            hval += asso_values[(unsigned char)s[6]];
            /*fall through*/
        case 6:
            hval += asso_values[(unsigned char)s[5]];
            /*fall through*/
        case 5:
            hval += asso_values[(unsigned char)s[4]];
            /*fall through*/
        case 4:
            hval += asso_values[(unsigned char)s[3]];
            /*fall through*/
        case 3:
            hval += asso_values[(unsigned char)s[2]];
            /*fall through*/
        case 2:
            hval += asso_values[(unsigned char)s[1]];
            /*fall through*/
        case 1:
            hval += asso_values[(unsigned char)s[0]];
            break;
    }
    return hval;
}

ti_field_map_t field__mapping[TOTAL_KEYWORDS] = {
    {.name="any",       .spec=TI_SPEC_ANY,      .dval_cb=field__dval_nil},
    {.name="thing",     .spec=TI_SPEC_OBJECT,   .dval_cb=field__dval_thing},
    {.name="raw",       .spec=TI_SPEC_RAW,      .dval_cb=field__dval_str},
    {.name="str",       .spec=TI_SPEC_STR,      .dval_cb=field__dval_str},
    {.name="utf8",      .spec=TI_SPEC_UTF8,     .dval_cb=field__dval_str},
    {.name="bytes",     .spec=TI_SPEC_BYTES,    .dval_cb=field__dval_bin},
    {.name="int",       .spec=TI_SPEC_INT,      .dval_cb=field__dval_int},
    {.name="uint",      .spec=TI_SPEC_UINT,     .dval_cb=field__dval_int},
    {.name="pint",      .spec=TI_SPEC_PINT,     .dval_cb=field__dval_pint},
    {.name="nint",      .spec=TI_SPEC_NINT,     .dval_cb=field__dval_nint},
    {.name="float",     .spec=TI_SPEC_FLOAT,    .dval_cb=field__dval_float},
    {.name="number",    .spec=TI_SPEC_NUMBER,   .dval_cb=field__dval_int},
    {.name="bool",      .spec=TI_SPEC_BOOL,     .dval_cb=field__dval_bool},
    {.name="[]",        .spec=TI_SPEC_ARR,      .dval_cb=field__dval_arr},
    {.name="{}",        .spec=TI_SPEC_SET,      .dval_cb=field__dval_set},
    {.name="datetime",  .spec=TI_SPEC_DATETIME, .dval_cb=field__dval_datetime},
    {.name="timeval",   .spec=TI_SPEC_TIMEVAL,  .dval_cb=field__dval_timeval},
    {.name="regex",     .spec=TI_SPEC_REGEX,    .dval_cb=field__dval_regex},
    {.name="closure",   .spec=TI_SPEC_CLOSURE,  .dval_cb=field__dval_closure},
    {.name="error",     .spec=TI_SPEC_ERROR,    .dval_cb=field__dval_error},
    {.name="room",      .spec=TI_SPEC_ROOM,     .dval_cb=field__dval_room},
    {.name="task",      .spec=TI_SPEC_TASK,     .dval_cb=field__dval_task},
    {.name="email",     .spec=TI_SPEC_EMAIL,    .dval_cb=field__dval_str},
    {.name="url",       .spec=TI_SPEC_URL,      .dval_cb=field__dval_str},
    {.name="tel",       .spec=TI_SPEC_TEL,      .dval_cb=field__dval_str},
};

static ti_field_map_t * field__map[MAX_HASH_VALUE+1];

/*
 * Initializes ThingsDB function mapping.
 *
 * Note: must be called exactly once when starting ThingsDB.
 */
void ti_field_init(void)
{
    for (size_t i = 0, n = TOTAL_KEYWORDS; i < n; ++i)
    {
        uint32_t key;
        ti_field_map_t * fmap = &field__mapping[i];

        fmap->n = strlen(fmap->name);
        key = field__hash(fmap->name, fmap->n);

        assert(field__map[key] == NULL);
        assert(key <= MAX_HASH_VALUE);

        field__map[key] = fmap;
    }
}

ti_field_map_t * ti_field_map_by_strn(const char * s, size_t n)
{
    register uint32_t key = field__hash(s, n);
    ti_field_map_t * fmap = key <= MAX_HASH_VALUE
            ? field__map[key] : NULL;
    return fmap && memcmp(s, fmap->name, n) == 0 ? fmap : NULL;
}

ti_field_map_t * ti_field_map_by_spec(const uint16_t spec)
{
    if (spec < TI_SPEC_ANY || spec >= TI_SPEC_ANY + TOTAL_KEYWORDS)
        return NULL;
    return &field__mapping[spec-TI_SPEC_ANY];
}

static int field__init(ti_field_t * field, ex_t * e)
{
    const char * str = (const char *) field->spec_raw->data;
    size_t n = field->spec_raw->n;
    uint16_t * spec = &field->spec;
    ti_field_map_t * fmap;

    field->flags = 0;
    field->spec = 0;
    field->nested_spec = 0;
    field->dval_cb = NULL;
    field->condition.none = NULL;

    if (!n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type declarations must not be empty"DOC_T_TYPE,
                field->name->str, field->type->name);
        return e->nr;
    }

    do
    {
        /* WARNING: do not forget to update types.c when modifying the flags in
         * this code; (types__spec_flags_pos)
         */
        switch(*str)
        {
        case '&':
            if (field->flags & TI_FIELD_FLAG_SAME_DEEP)
                goto duplicate_flag;
            field->flags |= TI_FIELD_FLAG_SAME_DEEP;
            break;
        case '-':
            if (field->flags & TI_FIELD_FLAG_MIN_DEEP)
                goto duplicate_flag;
            field->flags |= TI_FIELD_FLAG_MIN_DEEP;
            break;
        case '+':
            if (field->flags & TI_FIELD_FLAG_MAX_DEEP)
                goto duplicate_flag;
            field->flags |= TI_FIELD_FLAG_MAX_DEEP;
            break;
        case '^':
            if (field->flags & TI_FIELD_FLAG_NO_IDS)
                goto duplicate_flag;
            field->flags |= TI_FIELD_FLAG_NO_IDS;
            break;
        case '*':
            if (field->flags & TI_FIELD_FLAG_ENAME)
                goto duplicate_flag;
            field->flags |= TI_FIELD_FLAG_ENAME;
            break;
        default:
            goto done_flags;
        }
        ++str;
    }
    while (--n);

    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "missing declaration after flags"DOC_T_TYPE,
            field->name->str, field->type->name);
    return e->nr;

done_flags:
    switch (field->flags & TI_FIELD_MIN_MAX)
    {
    case TI_FIELD_MIN_MAX:
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "conflicting flags `-` and `+`"DOC_T_TYPE,
                field->name->str, field->type->name);
        return e->nr;
    case 0:
        field->flags |= TI_FIELD_FLAG_DEEP;  /* most common, first check */
    }

    if (str[n-1] == '?')
    {
        field->spec |= TI_SPEC_NILLABLE;
        field__set_cb(field, field__dval_nil);
        if (!--n)
            goto invalid;
    }

    if (*str == '[')
    {
        if (str[n-1] != ']')
            goto invalid;
        field->spec |= TI_SPEC_ARR;
        field__set_cb(field, field__dval_arr);
    }
    else if (*str == '{')
    {
        if (str[n-1] != '}')
            goto invalid;
        field->spec |= TI_SPEC_SET;
        field__set_cb(field, field__dval_set);
    }
    else if (n >= 7 && memcmp(str, "thing<", 6) == 0)
    {
        if (str[n-1] != '>')
            goto invalid;
        field->spec |= TI_SPEC_OBJECT;
        field__set_cb(field, field__dval_restricted);
        str += 5;
        n -= 5;
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
        return 0;  /* dval_cb is set to nil, array, set or restricted */
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

    assert(n);

    switch (*str)
    {
    case '[':
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `[`; nested array declarations are not allowed"
            DOC_T_TYPE, field->name->str, field->type->name);
        return e->nr;
    case '{':
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `{`; nested set declarations are not allowed"
            DOC_T_TYPE, field->name->str, field->type->name);
        return e->nr;
    case '/':
        return ti_condition_field_re_init(field, str, n, e);
    }

    if (str[n-1] == '>')
        return ti_condition_field_info_init(field, str, n, e);

    fmap = ti_field_map_by_strn(str, n);
    if (fmap)
    {
        if (fmap->spec == TI_SPEC_ANY)
            *spec = TI_SPEC_ANY;  /* overwrite any */
        else
            *spec |= fmap->spec;  /* append all the rest */

        field__set_cb(field, fmap->dval_cb);
        goto found;
    }

    if (field__cmp(str, n, field->type->name))
    {
        *spec |= field->type->type_id;
        if (&field->spec == spec && (~field->spec & TI_SPEC_NILLABLE))
            goto circular_dep;

        if (field->type->selfref == UINT8_MAX)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "invalid declaration for `%s` on type `%s`; "
                    "maximum number of self references has been reached"
                    DOC_T_TYPE,
                    field->name->str, field->type->name);
            return e->nr;
        }

        field__set_cb(field, field__dval_type);
        ++field->type->selfref;
    }
    else
    {
        ti_type_t * dep = ti_types_by_strn(field->type->types, str, n);
        if (!dep)
        {
            ti_enum_t * enum_ = ti_enums_by_strn(
                    field->type->types->collection->enums,
                    str,
                    n);

            if (!enum_)
            {
                if (field__spec_is_ascii(field, str, n, e))
                    ex_set(e, EX_TYPE_ERROR,
                            "invalid declaration for `%s` on type `%s`; "
                            "unknown type `%.*s` in declaration"DOC_T_TYPE,
                            field->name->str, field->type->name,
                            n, str);
                return e->nr;
            }

            /* assign the enum as dependency */
            dep = (ti_type_t *) enum_;

            if (enum_->flags & TI_ENUM_FLAG_LOCK)
            {
                ex_set(e, EX_OPERATION,
                    "invalid declaration for `%s` on type `%s`; "
                    "cannot assign enum type `%s` while the enum is in use"
                    DOC_T_TYPE,
                    field->name->str, field->type->name,
                    enum_->name);

                return e->nr;
            }

            if ((field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET)
            {
                ex_set(e, EX_TYPE_ERROR,
                    "invalid declaration for `%s` on type `%s`; "
                    "type `"TI_VAL_SET_S"` cannot contain enum type `%s`"
                    DOC_T_TYPE,
                    field->name->str, field->type->name,
                    enum_->name);
                return e->nr;
            }

            *spec |= enum_->enum_id | TI_ENUM_ID_FLAG;
            field__set_cb(field, field__dval_enum);
        }
        else
        {
            if (dep->flags & TI_TYPE_FLAG_LOCK)
            {
                ex_set(e, EX_OPERATION,
                    "invalid declaration for `%s` on type `%s`; "
                    "cannot assign type `%s` while the type is in use"
                    DOC_T_TYPE,
                    field->name->str, field->type->name,
                    dep->name);

                return e->nr;
            }

            *spec |= dep->type_id;
            field__set_cb(field, field__dval_type);

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
                ti_spec_approx_type_str(field->nested_spec));
        else if (field->nested_spec == TI_SPEC_OBJECT)
            field->nested_spec = TI_SPEC_ANY;
    }

    assert(field->dval_cb);  /* callback must have been set */

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
        field->spec_raw->n, (const char *) field->spec_raw->data);

    return e->nr;

circular_dep:
    ex_set(e, EX_VALUE_ERROR,
        "invalid declaration for `%s` on type `%s`; "
        "missing `?` after declaration `%.*s`; "
        "circular dependencies must be nillable "
        "at least at one point in the chain"DOC_T_TYPE,
        field->name->str, field->type->name,
        field->spec_raw->n, (const char *) field->spec_raw->data);
    return e->nr;

duplicate_flag:
    ex_set(e, EX_VALUE_ERROR,
        "invalid declaration for `%s` on type `%s`; "
        "duplicate flags"DOC_T_TYPE,
        field->name->str, field->type->name);
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
        assert(e->nr);        ;
        ti_field_destroy(vec_pop(type->fields));
        return NULL;
    }

    assert(e->nr == 0);
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
    assert(field->idx == (*with_field)->idx);
    assert(field->type == (*with_field)->type);

    field__remove_dep(field);

    ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
    ti_condition_destroy(field->condition, field->spec);

    field->spec = (*with_field)->spec;
    field->nested_spec = (*with_field)->nested_spec;
    field->spec_raw = (*with_field)->spec_raw;
    field->condition = (*with_field)->condition;
    field->dval_cb = (*with_field)->dval_cb;
    field->flags = (*with_field)->flags;

    (*with_field)->condition.none = NULL;

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
    ti_field_dval_cb prev_dval_cb = field->dval_cb;
    int prev_flags = field->flags;

    field__remove_dep(field);

    field->spec_raw = spec_raw;

    if (field__init(field, e))
        goto undo;

    ti_incref(spec_raw);
    ti_val_unsafe_drop((ti_val_t *) prev_spec_raw);
    ti_condition_destroy(prev_condition, prev_spec);
    return 0;

undo:
    field->spec_raw = prev_spec_raw;
    field->spec = prev_spec;
    field->nested_spec = prev_nested_spec;
    field->condition = prev_condition;
    field->dval_cb = prev_dval_cb;
    field->flags = prev_flags;
    (void) field__add_dep(field);

    return e->nr;
}

int ti_field_mod(
        ti_field_t * field,
        ti_raw_t * spec_raw,
        ex_t * e)
{
    ti_raw_t * prev_spec_raw = field->spec_raw;
    ti_condition_via_t prev_condition = field->condition;
    uint16_t prev_spec = field->spec;
    uint16_t prev_nested_spec = field->nested_spec;
    ti_field_dval_cb prev_dval_cb = field->dval_cb;
    int prev_flags = field->flags;

    field__remove_dep(field);

    field->spec_raw = spec_raw;
    if (field__init(field, e))
        goto undo;

    if (ti_type_is_wrap_only(field->type))
        goto done;

    switch (ti_spec_check_mod(
            prev_spec,
            field->spec,
            prev_condition,
            field->condition))
    {
    case TI_SPEC_MOD_SUCCESS:           goto done;
    case TI_SPEC_MOD_ERR:               goto incompatible;
    case TI_SPEC_MOD_NILLABLE_ERR:      goto nillable;
    case TI_SPEC_MOD_NESTED:
        switch (ti_spec_check_mod(
                prev_nested_spec,
                field->nested_spec,
                prev_condition,
                field->condition))
        {
        case TI_SPEC_MOD_SUCCESS:           goto done;
        case TI_SPEC_MOD_ERR:               goto incompatible;
        case TI_SPEC_MOD_NILLABLE_ERR:      goto nillable;
        case TI_SPEC_MOD_NESTED:            goto incompatible;
        }
    }

    assert(0);

nillable:
    ex_set(e, EX_OPERATION,
        "cannot apply type declaration `%.*s` to `%s` on type `%s` without a "
        "closure to migrate existing instances; the old declaration "
        "was nillable while the new declaration is not"DOC_MOD_TYPE_MOD,
        spec_raw->n, (const char *) spec_raw->data,
        field->name->str,
        field->type->name);
    goto undo_dep;

incompatible:
    ex_set(e, EX_OPERATION,
        "cannot apply type declaration `%.*s` to `%s` on type `%s` without a "
        "closure to migrate existing instances; the old declaration `%.*s` "
        "is not compatible with the new declaration"DOC_MOD_TYPE_MOD,
        spec_raw->n, (const char *) spec_raw->data,
        field->name->str,
        field->type->name,
        prev_spec_raw->n, (const char *) prev_spec_raw->data);

undo_dep:
    field__remove_dep(field);
    ti_condition_destroy(field->condition, field->spec);

undo:
    field->spec_raw = prev_spec_raw;
    field->spec = prev_spec;
    field->nested_spec = prev_nested_spec;
    field->condition = prev_condition;
    field->dval_cb = prev_dval_cb;
    field->flags = prev_flags;
    (void) field__add_dep(field);

    return e->nr;

done:
    ti_incref(spec_raw);
    ti_val_unsafe_drop((ti_val_t *) prev_spec_raw);
    ti_condition_destroy(prev_condition, prev_spec);
    return 0;
}

int ti_field_set_name(
        ti_field_t * field,
        const char * s,
        size_t n,
        ex_t * e)
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

    if (field->type->idname == name ||
        ti_field_by_name(field->type, name) ||
        ti_type_get_method(field->type, name))
    {
        ex_set(e, EX_VALUE_ERROR,
            "property or method `%s` already exists on type `%s`"DOC_T_TYPED,
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

int ti_field_del(ti_field_t * field)
{
    vec_t * vec = imap_vec_ref(field->type->types->collection->things);
    uint16_t type_id = field->type->type_id;
    if (!vec)
        return -1;

    for (vec_each(vec, ti_thing_t, thing))
    {
        if (thing->type_id == type_id)
            ti_val_unsafe_drop(vec_swap_remove(thing->items.vec, field->idx));

        ti_val_unsafe_drop((ti_val_t *) thing);
    }

    /*
     * The garbage collector has a reference to each thing so things can never
     * be removed by a field which is removed.
     */
    for (queue_each(field->type->types->collection->gc, ti_gc_t, gc))
    {
        if (gc->thing->type_id == type_id)
            ti_val_unsafe_drop(vec_swap_remove(
                    gc->thing->items.vec,
                    field->idx));
    }

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

    swap = vec_get(field->type->fields, field->idx);
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

typedef struct
{
    ti_field_t * field;
    ti_thing_t * relation;
    imap_t * imap;
} field__walk_set_t;

static int field__walk_unset_cb(ti_thing_t * thing, field__walk_set_t * w)
{
    w->field->condition.rel->del_cb(w->field, thing, w->relation);
    return 0;
}

static int field__walk_set_cb(ti_thing_t * thing, field__walk_set_t * w)
{
    if (w->imap && imap_pop(w->imap, ti_thing_key(thing)))
    {
        ti_decref(thing);
        return 0;
    }

    w->field->condition.rel->add_cb(w->field, thing, w->relation);
    return 0;
}

static int field__walk_tset_cb(ti_thing_t * thing, field__walk_set_t * w)
{
    ti_thing_t * other;

    if (w->imap && imap_pop(w->imap, ti_thing_key(thing)))
    {
        ti_decref(thing);
        return 0;
    }

    other = VEC_get(thing->items.vec, w->field->idx);
    if (other->tp == TI_VAL_THING)
    {
        ti_field_t * ofield = w->field->condition.rel->field;
        ofield->condition.rel->del_cb(ofield, other, thing);
    }

    w->field->condition.rel->add_cb(w->field, thing, w->relation);
    return 0;
}

int ti_field_vset_pre_assign(
        ti_field_t * field,
        imap_t * imap,
        ti_thing_t * parent,
        ex_t * e,
        _Bool do_type_check)
{
    /*
     * This method may be called when field is either `any` or a `set.
     * In case of `any`, we have to make sure the specification will be
     * OBJECT, not ANY.
     */
    ti_vset_t * oset;
    _Bool with_relation = field->condition.rel && parent;

    if (field->nested_spec == TI_SPEC_ANY || imap->n == 0)
        goto done;

    if (do_type_check && imap_walk(imap, (imap_cb) field__walk_assign, field))
    {
        ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` has definition `%.*s` but got a set with "
                "at least one thing of another type",
                field->type->name,
                field->name->str,
                field->spec_raw->n, (const char *) field->spec_raw->data);
        return e->nr;
    }

    if (with_relation && !parent->id)
    {
        ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s` on property `%s`; "
                "relations must be created using a property "
                "on a stored thing (a thing with an Id)",
                field->type->name,
                field->name->str);
        return e->nr;
    }

done:
    if (with_relation)
    {
        oset = VEC_get(parent->items.vec, field->idx);
        field__walk_set_t w = {
                .field = field->condition.rel->field,
                .relation = parent,
                .imap = oset->imap,
        };

        (void) imap_walk(imap, w.field->spec == TI_SPEC_SET
                ? (imap_cb) field__walk_set_cb
                : (imap_cb) field__walk_tset_cb, &w);

        w.imap = imap;
        (void) imap_walk(oset->imap, (imap_cb) field__walk_unset_cb, &w);
    }
    return 0;
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
    ti_vset_t * oset;
    _Bool with_relation = field->condition.rel && parent;

    if (field->nested_spec == TI_SPEC_ANY ||
        field->nested_spec == ti_vset_spec(*vset) ||
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
                field->spec_raw->n, (const char *) field->spec_raw->data);
        return e->nr;
    }

    if (with_relation && !parent->id)
    {
        ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s` on property `%s`; "
                "relations must be created using a property "
                "on a stored thing (a thing with an Id)",
                field->type->name,
                field->name->str);
        return e->nr;
    }

done:
    if (ti_vset_assign(vset))
    {
        ex_set_mem(e);
        return e->nr;
    }
    (*vset)->parent = parent;
    (*vset)->key_ = field;

    if (with_relation)
    {
        oset = vec_get(parent->items.vec, field->idx);

        field__walk_set_t w = {
                .field = field->condition.rel->field,
                .relation = parent,
                .imap = oset ? oset->imap : NULL,
        };

        imap_walk((*vset)->imap, w.field->spec == TI_SPEC_SET
                ? (imap_cb) field__walk_set_cb
                : (imap_cb) field__walk_tset_cb, &w);

        if (oset)
        {
            w.imap = (*vset)->imap;
            (void) imap_walk(oset->imap, (imap_cb) field__walk_unset_cb, &w);
        }
    }

    return 0;
}

static int field__varr_assign(
        ti_field_t * field,
        ti_varr_t ** varr,
        ti_thing_t * parent,
        ex_t * e)
{
    if (field->nested_spec == TI_SPEC_ANY ||
        field->nested_spec == ti_varr_spec(*varr) ||
        (*varr)->vec->n == 0)
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
                field->spec_raw->n, (const char *) field->spec_raw->data);
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
        case TI_SPEC_RVAL_EMAIL_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with email addresses",
                field->type->name,
                field->name->str);
            return e->nr;
        case TI_SPEC_RVAL_URL_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with URL's",
                field->type->name,
                field->name->str);
            return e->nr;
        case TI_SPEC_RVAL_TEL_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with telephone numbers",
                field->type->name,
                field->name->str);
            return e->nr;
        }
    }

done:
    return ti_val_make_assignable((ti_val_t **) varr, parent, field, e);
}

static int field__restrict_cb(ti_prop_t * prop, ti_field_t * field)
{
    return ti_spec_check_nested_val(field->nested_spec, prop->val);
}

static int field__thing_assign(
        ti_field_t * field,
        ti_thing_t * thing,
        ex_t * e)
{
    ti_spec_rval_enum rc = TI_SPEC_RVAL_SUCCESS;

    if (ti_thing_is_instance(thing))
    {
        ex_set(e, EX_TYPE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a non-typed thing but got type `%s`",
            field->type->name,
            field->name->str,
            thing->via.type->name);
        return e->nr;
    }

    if (thing->via.spec == field->nested_spec)
        return 0;  /* already the appropriate restriction */

    if (thing->via.spec != TI_SPEC_ANY)
    {
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s` on property `%s`; "
            "restriction mismatch",
            field->type->name,
            field->name->str);
        return e->nr;
    }

    if (ti_thing_n(thing) == 0)
    {
        thing->via.spec = field->nested_spec;
        return 0;
    }

    if (ti_thing_is_dict(thing))
    {
        rc = (ti_spec_rval_enum) smap_values(
                thing->items.smap,
                (smap_val_cb) field__restrict_cb,
                field);
    }
    else for (vec_each(thing->items.vec, ti_prop_t, prop))
        if ((rc = ti_spec_check_nested_val(field->nested_spec, prop->val)))
            break;

    switch (rc)
    {
    case TI_SPEC_RVAL_SUCCESS:
        thing->via.spec = field->nested_spec;
        return 0;
    case TI_SPEC_RVAL_TYPE_ERROR:
        ex_set(e, EX_TYPE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with values that matches "
            "definition `%.*s`",
            field->type->name,
            field->name->str,
            field->spec_raw->n, (const char *) field->spec_raw->data);
        return e->nr;
    case TI_SPEC_RVAL_UTF8_ERROR:
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with UTF8 string values",
            field->type->name,
            field->name->str);
        return e->nr;
    case TI_SPEC_RVAL_UINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with integer values "
            "greater than or equal to 0",
            field->type->name,
            field->name->str);
        return e->nr;
    case TI_SPEC_RVAL_PINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with positive integer values",
            field->type->name,
            field->name->str);
        return e->nr;
    case TI_SPEC_RVAL_NINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with negative integer values",
            field->type->name,
            field->name->str);
        return e->nr;
    case TI_SPEC_RVAL_EMAIL_ERROR:
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with email addresses",
            field->type->name,
            field->name->str);
        return e->nr;
    case TI_SPEC_RVAL_URL_ERROR:
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with URL's",
            field->type->name,
            field->name->str);
        return e->nr;
    case TI_SPEC_RVAL_TEL_ERROR:
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires a thing with telephone numbers",
            field->type->name,
            field->name->str);
        return e->nr;
    }
    assert(0);
    return 0;
}

static _Bool field__maps_arr_to_arr(ti_field_t * field, ti_varr_t * varr)
{
    if (field->nested_spec == TI_SPEC_ANY ||
        varr->vec->n == 0 ||
        field->nested_spec == ti_varr_spec(varr))
        return true;

    for (vec_each(varr->vec, ti_val_t, val))
        if (!ti_spec_maps_to_nested_val(field->nested_spec, val))
            return false;

    return true;
}

static int field__map_restrict_cb(ti_prop_t * prop, ti_field_t * field)
{
    return !ti_spec_maps_to_nested_val(field->nested_spec, prop->val);
}

static _Bool field__maps_restricted(ti_field_t * field, ti_thing_t * thing)
{
    if (field->nested_spec == TI_SPEC_ANY)
        return true;

    if (ti_thing_is_instance(thing))
        return false;

    if (thing->via.spec == field->nested_spec)
        return true;

    if (ti_thing_is_dict(thing))
        return !smap_values(
                thing->items.smap,
                (smap_val_cb) field__map_restrict_cb,
                field);

    for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (!ti_spec_maps_to_nested_val(field->nested_spec, prop->val))
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
    uint16_t spec = field->spec & TI_SPEC_MASK_NILLABLE;

    if (spec < TI_SPEC_ANY)
    {
        /*
         * Just compare the specification with the type since the nillable
         * mask is removed the specification
         */
        if (parent && field->condition.rel)
        {
            ti_thing_t * relation = vec_get(parent->items.vec, field->idx);
            ti_field_t * rfield = field->condition.rel->field;

            if (!ti_val_is_nil(*val) && (!ti_val_is_thing(*val) ||
                ((ti_thing_t *) *val)->type_id != spec))
                goto type_error;

            if (!parent->id && ti_val_is_thing(*val))
                goto relation_error;

            if (relation && relation->tp == TI_VAL_THING)
                rfield->condition.rel->del_cb(rfield, relation, parent);

            if (ti_val_is_thing(*val))
            {
                relation = (ti_thing_t *) *val;
                if (rfield->spec != TI_SPEC_SET)
                {
                    ti_thing_t * prev = \
                            VEC_get(relation->items.vec, rfield->idx);
                    if (prev->tp == TI_VAL_THING)
                        field->condition.rel->del_cb(field, prev, relation);
                }

                rfield->condition.rel->add_cb(rfield, relation, parent);
            }

            return 0;
        }

        if (((field->spec & TI_SPEC_NILLABLE) && ti_val_is_nil(*val)) ||
            (ti_val_is_thing(*val) && ((ti_thing_t *) *val)->type_id == spec))
            return 0;

        goto type_error;
    }

    if ((field->spec & TI_SPEC_NILLABLE) && ti_val_is_nil(*val))
        return 0;

    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        switch ((ti_val_enum) (*val)->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_DATETIME:
        case TI_VAL_MPDATA:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ROOM:
        case TI_VAL_TASK:
            break;
        case TI_VAL_ARR:
            return field__varr_assign(field, (ti_varr_t **) val, parent, e);
        case TI_VAL_SET:
            return field__vset_assign(field, (ti_vset_t **) val, parent, e);
        case TI_VAL_CLOSURE:
            return ti_closure_unbound((ti_closure_t *) *val, e);
        case TI_VAL_ERROR:
        case TI_VAL_MEMBER:
        case TI_VAL_TEMPLATE:
            break;
        case TI_VAL_FUTURE:
            goto future_error;
        }
        return 0;
    case TI_SPEC_OBJECT:
        if (ti_val_is_thing(*val))
            return field->nested_spec == TI_SPEC_ANY
                    ? 0
                    : field__thing_assign(field, (ti_thing_t *) *val, e);
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
    case TI_SPEC_DATETIME:
        if (ti_val_is_datetime_strict(*val))
            return 0;
        goto type_error;
    case TI_SPEC_TIMEVAL:
        if (ti_val_is_timeval(*val))
            return 0;
        goto type_error;
    case TI_SPEC_REGEX:
        if (ti_val_is_regex(*val))
            return 0;
        goto type_error;
    case TI_SPEC_CLOSURE:
        if (ti_val_is_closure(*val))
            return ti_closure_unbound((ti_closure_t *) *val, e);
        goto type_error;
    case TI_SPEC_ERROR:
        if (ti_val_is_error(*val))
            return 0;
        goto type_error;
    case TI_SPEC_ROOM:
        if (ti_val_is_room(*val))
            return 0;
        goto type_error;
    case TI_SPEC_TASK:
        if (ti_val_is_task(*val))
            return 0;
        goto type_error;
    case TI_SPEC_EMAIL:
        if (!ti_val_is_str(*val))
            goto type_error;
        if (!ti_regex_test_or_empty(
                (ti_regex_t *) ti_val_borrow_re_email(),
                (ti_raw_t *) *val))
            goto email_error;
        return 0;
    case TI_SPEC_URL:
        if (!ti_val_is_str(*val))
            goto type_error;
        if (!ti_regex_test_or_empty(
                (ti_regex_t *) ti_val_borrow_re_url(),
                (ti_raw_t *) *val))
            goto url_error;
        return 0;
    case TI_SPEC_TEL:
        if (!ti_val_is_str(*val))
            goto type_error;
        if (!ti_regex_test_or_empty(
                (ti_regex_t *) ti_val_borrow_re_tel(),
                (ti_raw_t *) *val))
            goto tel_error;
        return 0;
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
    case TI_SPEC_UTF8_RANGE:
        if (!ti_val_is_str(*val))
            goto type_error;
        if (!strx_is_utf8n(
                (const char *) ((ti_raw_t *) *val)->data,
                ((ti_raw_t *) *val)->n))
            goto utf8_error;
        if (((ti_raw_t *) *val)->n < field->condition.srange->mi ||
            ((ti_raw_t *) *val)->n > field->condition.srange->ma)
            goto srange_error;
        return 0;
    }

    assert(spec >= TI_ENUM_ID_FLAG);

    if (ti_spec_enum_eq_to_val(spec, *val))
        return 0;

    goto type_error;

future_error:
    ex_set(e, EX_TYPE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` allows `any` type with the exception "
            "of the `future` type",
            field->type->name,
            field->name->str);
    return e->nr;

irange_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` requires an integer value between "
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
    if (field->condition.srange->ma == INT64_MAX)
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires a string with a length of at least "
                "%zu",
                field->type->name,
                field->name->str,
                field->condition.srange->mi);
    else if (field->condition.srange->mi == field->condition.srange->ma)
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires a string with a length "
                "of %zu",
                field->type->name,
                field->name->str,
                field->condition.srange->mi);
    else
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires a string with a length "
                "between %zu and %zu (both inclusive)",
                field->type->name,
                field->name->str,
                field->condition.srange->mi,
                field->condition.srange->ma);
    return e->nr;

re_error:
    if (field->condition.re->regex == (ti_regex_t *) ti_val_borrow_re_email())
        goto email_error;
    else if (field->condition.re->regex == (ti_regex_t *) ti_val_borrow_re_url())
        goto url_error;
    else if (field->condition.re->regex == (ti_regex_t *) ti_val_borrow_re_tel())
        goto tel_error;
    else
        ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` has a requirement to match pattern %.*s",
            field->type->name,
            field->name->str,
            field->condition.re->regex->pattern->n,
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

email_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` only accepts an email address",
            field->type->name,
            field->name->str);
    return e->nr;

url_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` only accepts a URL",
            field->type->name,
            field->name->str);
    return e->nr;

tel_error:
    ex_set(e, EX_VALUE_ERROR,
            "mismatch in type `%s`; "
            "property `%s` only accepts a telephone number",
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
            field->spec_raw->n, (const char *) field->spec_raw->data);
    return e->nr;

relation_error:
    ex_set(e, EX_TYPE_ERROR,
            "mismatch in type `%s` on property `%s`; "
            "relations must be created using a property "
            "on a stored thing (a thing with an Id)",
            field->type->name,
            field->name->str);
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
        return (ti_val_is_thing(val) &&
                field__maps_restricted(field, (ti_thing_t *) val));
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
    case TI_SPEC_DATETIME:
        return ti_val_is_datetime_strict(val);
    case TI_SPEC_TIMEVAL:
        return ti_val_is_timeval(val);
    case TI_SPEC_REGEX:
        return ti_val_is_regex(val);
    case TI_SPEC_CLOSURE:
        return ti_val_is_closure(val);
    case TI_SPEC_ERROR:
        return ti_val_is_error(val);
    case TI_SPEC_ROOM:
        return ti_val_is_room(val);
    case TI_SPEC_TASK:
        return ti_val_is_task(val);
    case TI_SPEC_EMAIL:
        return ti_val_is_str(val) && ti_regex_test_or_empty(
                (ti_regex_t *) ti_val_borrow_re_email(),
                (ti_raw_t *) val);
    case TI_SPEC_URL:
        return ti_val_is_str(val) && ti_regex_test_or_empty(
                (ti_regex_t *) ti_val_borrow_re_url(),
                (ti_raw_t *) val);
    case TI_SPEC_TEL:
        return ti_val_is_str(val) && ti_regex_test_or_empty(
                (ti_regex_t *) ti_val_borrow_re_tel(),
                (ti_raw_t *) val);
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
    case TI_SPEC_UTF8_RANGE:
        return (ti_val_is_str(val) && strx_is_utf8n(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n) &&
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
    assert(ti_spec_is_arr_or_set(f_field->spec));
    assert(ti_spec_is_arr_or_set(t_field->spec));

    if (t_field->nested_spec == TI_SPEC_ANY)
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
        return  f_spec < TI_SPEC_ANY || ti_spec_is_set(f_field->spec);
    case TI_SPEC_RAW:
        return (f_spec == TI_SPEC_STR ||
                f_spec == TI_SPEC_UTF8 ||
                f_spec == TI_SPEC_BYTES ||
                f_spec == TI_SPEC_EMAIL ||
                f_spec == TI_SPEC_URL ||
                f_spec == TI_SPEC_TEL);
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
        return (f_spec == TI_SPEC_UTF8 ||
                f_spec == TI_SPEC_EMAIL ||
                f_spec == TI_SPEC_URL ||
                f_spec == TI_SPEC_TEL);
    case TI_SPEC_UTF8:
    case TI_SPEC_BYTES:
    case TI_SPEC_PINT:
    case TI_SPEC_NINT:
    case TI_SPEC_FLOAT:
    case TI_SPEC_BOOL:
    case TI_SPEC_DATETIME:
    case TI_SPEC_TIMEVAL:
    case TI_SPEC_REGEX:
    case TI_SPEC_CLOSURE:
    case TI_SPEC_ERROR:
    case TI_SPEC_ROOM:
    case TI_SPEC_TASK:
    case TI_SPEC_EMAIL:
    case TI_SPEC_URL:
    case TI_SPEC_TEL:
    case TI_SPEC_ARR:
    case TI_SPEC_SET:
    case TI_SPEC_REMATCH:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
    case TI_SPEC_STR_RANGE:
    case TI_SPEC_UTF8_RANGE:
        return false;
    }

    assert(t_spec < TI_SPEC_ANY);  /* enumerators are already checked */

    return f_spec < TI_SPEC_ANY ||
           f_spec == TI_SPEC_OBJECT ||
           ti_spec_is_set(f_field->spec);
}

_Bool ti_field_maps_to_field(ti_field_t * t_field, ti_field_t * f_field)
{
    uint16_t t_spec, f_spec;
    assert(t_field->name == f_field->name);

    /* return 0 when `to` accepts `any` */
    if (t_field->spec == TI_SPEC_ANY)
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
                f_spec == TI_SPEC_EMAIL ||
                f_spec == TI_SPEC_URL ||
                f_spec == TI_SPEC_TEL ||
                f_spec == TI_SPEC_REMATCH ||
                f_spec == TI_SPEC_STR_RANGE ||
                f_spec == TI_SPEC_UTF8_RANGE);
    case TI_SPEC_STR:
        return (f_spec == TI_SPEC_STR ||
                f_spec == TI_SPEC_UTF8 ||
                f_spec == TI_SPEC_EMAIL ||
                f_spec == TI_SPEC_URL ||
                f_spec == TI_SPEC_TEL ||
                f_spec == TI_SPEC_REMATCH ||
                f_spec == TI_SPEC_STR_RANGE ||
                f_spec == TI_SPEC_UTF8_RANGE);
    case TI_SPEC_UTF8:
        return (f_spec == TI_SPEC_UTF8 ||
                f_spec == TI_SPEC_UTF8_RANGE);
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
    case TI_SPEC_DATETIME:
    case TI_SPEC_TIMEVAL:
    case TI_SPEC_REGEX:
    case TI_SPEC_CLOSURE:
    case TI_SPEC_ERROR:
    case TI_SPEC_ROOM:
    case TI_SPEC_TASK:
    case TI_SPEC_EMAIL:
    case TI_SPEC_URL:
    case TI_SPEC_TEL:
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
            (f_spec == TI_SPEC_STR_RANGE || f_spec == TI_SPEC_UTF8_RANGE) &&
            f_field->condition.srange->mi >= t_field->condition.srange->mi &&
            f_field->condition.srange->ma <= t_field->condition.srange->ma);
    case TI_SPEC_UTF8_RANGE:
        return (
            f_spec == TI_SPEC_UTF8_RANGE &&
            f_field->condition.srange->mi >= t_field->condition.srange->mi &&
            f_field->condition.srange->ma <= t_field->condition.srange->ma);
    }

    assert(t_spec < TI_SPEC_ANY);

    return f_spec < TI_SPEC_ANY || f_spec == TI_SPEC_OBJECT;
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
                n, str);
    else
        ex_set(e, EX_VALUE_ERROR,
                "property name must follow the naming rules"DOC_NAMES);

    return NULL;
}

typedef struct
{
    ti_field_t * field;
    ti_val_t ** vaddr;
    uint16_t type_id;
    ex_t e;
} field__add_t;

static int field__add(ti_thing_t * thing, field__add_t * w)
{
    if (thing->type_id != w->type_id)
        return 0;

    /* closure is already unbound, so only a memory exception can occur */
    if (ti_val_make_assignable(w->vaddr, thing, w->field, &w->e) ||
        vec_push(&thing->items.vec, *w->vaddr))
        return 1;

    ti_incref(*w->vaddr);
    return 0;
}

/*
 * Use only when adding a new field, to update the existing things;
 *
 * warn: ti_val_gen_ids() must have used on the given value before using this
 *       function
 * error: can only fail in case of a memory allocation error
 */
int ti_field_init_things(ti_field_t * field, ti_val_t ** vaddr)
{
    assert(field == vec_last(field->type->fields));
    int rc;
    field__add_t addjob = {
            .field = field,
            .vaddr = vaddr,
            .type_id = field->type->type_id,
            .e = {0}
    };

    rc = imap_walk(
            field->type->types->collection->things,
            (imap_cb) field__add,
            &addjob);
    rc = rc ? rc : ti_gc_walk(
            field->type->types->collection->gc,
            (queue_cb) field__add,
            &addjob);

    return rc;
}

typedef struct
{
    ti_field_t * field;
    ti_field_t * ofield;
    ex_t * e;
} field__type_rel_chk_t;

static int field__type_rel_chk(
        ti_thing_t * thing,
        ti_field_t * field,
        ti_field_t * ofield,
        ex_t * e)
{
    ti_thing_t * me, * othing = VEC_get(thing->items.vec, field->idx);
    if (othing->tp == TI_VAL_NIL)
        return 0;

    me = VEC_get(othing->items.vec, ofield->idx);
    if (me->tp == TI_VAL_NIL || me == thing)
        return 0;

    if (thing->id)
        ex_set(e, EX_TYPE_ERROR,
            "failed to create relation; "
            "property `%s` on "TI_THING_ID" is referring to "TI_THING_ID
            " while property `%s` on "TI_THING_ID" is "
            "referring to "TI_THING_ID,
            field->name->str, thing->id, othing->id,
            ofield->name->str, othing->id, me->id);
    else
        ex_set(e, EX_TYPE_ERROR,
            "failed to create relation; "
            "property `%s` on a `thing` is referring to a second `thing` "
            "while property `%s` on that second thing is referring to "
            "a third `thing`",
            field->name->str, ofield->name->str);
    return e->nr;
}

static int field__type_rel_chk_cb(ti_thing_t * thing, field__type_rel_chk_t * w)
{
    /* check type_id, as here we may get an object.
     * (Objects do not use via.type)
     */
    if (thing->type_id == w->field->type->type_id &&
        field__type_rel_chk(thing, w->field, w->ofield, w->e))
        return w->e->nr;

    /* the fields may be different but of the same type, therefore
     * the code must bubble down and also check the "set" below.
     */
    return thing->type_id == w->ofield->type->type_id
        ? field__type_rel_chk(thing, w->ofield, w->field, w->e)
        : 0;
}

typedef struct
{
    imap_t * collect;
    ti_field_t * field;
    ti_field_t * ofield;
    ti_thing_t * parent;
    ex_t * e;
} field__set_rel_chk_t;

static int field__set_rel_cb(ti_thing_t * thing, field__set_rel_chk_t * w)
{
    ti_thing_t * other = VEC_get(thing->items.vec, w->field->idx);
    if (other->tp == TI_VAL_THING && other != w->parent)
    {
        if (thing->id)
            ex_set(w->e, EX_TYPE_ERROR,
                    "failed to create relation; "
                    "thing "TI_THING_ID" belongs to a set `%s` on "TI_THING_ID
                    " but is referring to "TI_THING_ID,
                    thing->id, w->ofield->name->str, w->parent->id, other->id);
        else
            ex_set(w->e, EX_TYPE_ERROR,
                    "failed to create relation; "
                    "at least one thing belongs to a set (`%s.%s`) of a "
                    "different thing than the thing it is referring "
                    "to (`%s.%s`)",
                    w->ofield->type->name, w->ofield->name->str,
                    w->field->type->name, w->field->name->str);
        return w->e->nr;
    }

    switch (imap_add(w->collect, ti_thing_key(thing), thing))
    {
    case IMAP_SUCCESS:
        return 0;
    case IMAP_ERR_EXIST:
        if (thing->id)
            ex_set(w->e, EX_TYPE_ERROR,
                    "failed to create relation; "
                    "thing "TI_THING_ID" belongs to at least two "
                    "different sets; (property `%s` on type `%s`)",
                    thing->id, w->ofield->name->str, w->ofield->type->name);
        else
            ex_set(w->e, EX_TYPE_ERROR,
                    "failed to create relation; "
                    "at least one thing belongs to at least two "
                    "different sets; (property `%s` on type `%s`)",
                    w->ofield->name->str, w->ofield->type->name);
        return w->e->nr;
    case IMAP_ERR_ALLOC:
        ex_set_mem(w->e);
    }
    return w->e->nr;
}

static int field__set_rel_chk_cb(ti_thing_t * thing, field__set_rel_chk_t * w)
{
    if (thing->type_id == w->ofield->type->type_id)
    {
        w->parent = thing;
        ti_vset_t * vset = VEC_get(thing->items.vec, w->ofield->idx);
        return imap_walk(vset->imap, (imap_cb) field__set_rel_cb, w);
    }
    return 0;
}

static int field__walk_things(
        vec_t * vars,
        ti_collection_t * collection,
        imap_cb cb,
        void * w)
{
    return (
        (vars && ti_query_vars_walk(vars, collection, (imap_cb) cb, w)) ||
        imap_walk(collection->things, (imap_cb) cb, w) ||
        ti_gc_walk(collection->gc, (queue_cb) cb, w)
    );
}

static int field__non_id_chk(ti_thing_t * thing, field__set_rel_chk_t * w)
{
    if (thing->id)
        return 0;

    ex_set(w->e, EX_TYPE_ERROR,
            "failed to create relation; "
            "relations must be created using a property "
            "on a stored thing (a thing with an Id)");
    return w->e->nr;
}

int ti_field_relation_check(
        ti_field_t * field,
        ti_field_t * ofield,
        vec_t * vars,
        ex_t * e)
{
    ti_collection_t * collection = field->type->types->collection;

    if (vars)
    {
        field__set_rel_chk_t w = {
                .collect = NULL,
                .field = field,
                .ofield = ofield,
                .e = e,
        };
        if (ti_query_vars_walk(
                vars,
                collection,
                (imap_cb) field__non_id_chk,
                &w))
            return e->nr;
    }

    if (field->spec == TI_SPEC_SET && ofield->spec == TI_SPEC_SET)
        return 0;

    if (field->spec != TI_SPEC_SET && ofield->spec != TI_SPEC_SET)
    {
        field__type_rel_chk_t w = {
                .field = field,
                .ofield = ofield,
                .e = e,
        };

        if (field__walk_things(
                vars,
                collection,
                (imap_cb) field__type_rel_chk_cb,
                &w) && !e->nr)
            ex_set_mem(e);
        return e->nr;
    }

    if (field->spec == TI_SPEC_SET)
    {
        ti_field_t * tmp = field;
        field = ofield;
        ofield = tmp;
    }

    field__set_rel_chk_t w = {
            .collect = imap_create(),
            .field = field,
            .ofield = ofield,
            .e = e,
    };

    if (!w.collect)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (field__walk_things(
            vars,
            collection,
            (imap_cb) field__set_rel_chk_cb,
            &w) && !e->nr)
        ex_set_mem(e);

    imap_destroy(w.collect, NULL);
    return e->nr;
}

typedef struct
{
    ti_field_t * field;
    ti_field_t * ofield;
} field__rel_t;

typedef struct
{
    ti_field_t * field;
    ti_thing_t * relation;
} field__rel_set_t;

static int field__rel_set_add(ti_thing_t * thing, field__rel_set_t * w)
{
    w->field->condition.rel->add_cb(w->field, thing, w->relation);
    return 0;
}

static int field__rel_set_cb(ti_thing_t * thing, field__rel_t * w)
{
    if (thing->type_id == w->field->type->type_id)
    {
        ti_vset_t * vset = VEC_get(thing->items.vec, w->field->idx);
        field__rel_set_t r = {
                .field = w->ofield,
                .relation = thing,
        };
        (void) imap_walk(vset->imap, (imap_cb) field__rel_set_add, &r);
        /* the fields may be different but of the same type, therefore
         * the code must bubble down and also check the "set" below.
         */
    }

    if (thing->type_id == w->ofield->type->type_id)
    {
        ti_vset_t * vset = VEC_get(thing->items.vec, w->ofield->idx);
        field__rel_set_t r = {
                .field = w->field,
                .relation = thing,
        };
        return imap_walk(vset->imap, (imap_cb) field__rel_set_add, &r);
    }

    return 0;
}

static int field__rel_type_cb(ti_thing_t * thing, field__rel_t * w)
{
    if (thing->type_id == w->field->type->type_id)
    {
        ti_thing_t * relation = VEC_get(thing->items.vec, w->field->idx);
        if (relation->tp == TI_VAL_THING)
            w->ofield->condition.rel->add_cb(w->ofield, relation, thing);
        /* the fields may be different but of the same type, therefore
         * the code must bubble down and also check the "set" below.
         */
    }

    if (thing->type_id == w->ofield->type->type_id)
    {
        ti_thing_t * relation = VEC_get(thing->items.vec, w->ofield->idx);
        if (relation->tp == TI_VAL_THING)
            w->field->condition.rel->add_cb(w->field, relation, thing);
    }
    return 0;
}

static int field__rel_st_cb(ti_thing_t * thing, field__rel_t * w)
{
    if (thing->type_id == w->field->type->type_id)
    {
        ti_thing_t * relation = VEC_get(thing->items.vec, w->field->idx);
        if (relation->tp == TI_VAL_THING)
            w->ofield->condition.rel->add_cb(w->ofield, relation, thing);
        /* the fields may be different but of the same type, therefore
         * the code must bubble down and also check the "set" below.
         */
    }

    if (thing->type_id == w->ofield->type->type_id)
    {
        ti_vset_t * vset = VEC_get(thing->items.vec, w->ofield->idx);
        field__rel_set_t r = {
                .field = w->field,
                .relation = thing,
        };
        return imap_walk(vset->imap, (imap_cb) field__rel_set_add, &r);
    }
    return 0;
}

int ti_field_relation_make(
        ti_field_t * field,
        ti_field_t * ofield,
        vec_t * vars)   /* may be NULL */
{
    ti_collection_t * collection = field->type->types->collection;
    field__rel_t w = {
           .field = field,
           .ofield = ofield,
    };

    if (field->spec == TI_SPEC_SET && ofield->spec == TI_SPEC_SET)
    {
        return field__walk_things(
                vars,
                collection,
                (imap_cb) field__rel_set_cb,
                &w);
    }

    if (field->spec != TI_SPEC_SET && ofield->spec != TI_SPEC_SET)
    {
        return field__walk_things(
                vars,
                collection,
                (imap_cb) field__rel_type_cb,
                &w);
    }

    if (field->spec == TI_SPEC_SET)
    {
        w.ofield = field;
        w.field = ofield;
    }

    return field__walk_things(
            vars,
            collection,
            (imap_cb) field__rel_st_cb,
            &w);
}
