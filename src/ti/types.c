/*
 * ti/types.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/spec.inline.h>
#include <ti/type.h>
#include <ti/types.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>


ti_types_t * ti_types_create(ti_collection_t * collection)
{
    ti_types_t * types = malloc(sizeof(ti_types_t));
    if (!types)
        return NULL;

    types->imap = imap_create();
    types->smap = smap_create();
    types->removed = smap_create();
    types->collection = collection;
    types->next_id = 0;

    if (!types->imap || !types->smap || !types->removed)
    {
        ti_types_destroy(types);
        return NULL;
    }

    return types;
}

void ti_types_destroy(ti_types_t * types)
{
    if (!types)
        return;

    smap_destroy(types->smap, NULL);
    smap_destroy(types->removed, NULL);
    imap_destroy(types->imap, (imap_destroy_cb) ti_type_destroy);
    free(types);
}

int ti_types_add(ti_types_t * types, ti_type_t * type)
{
    if (imap_add(types->imap, type->type_id, type))
        return -1;

    if (smap_add(types->smap, type->name, type))
    {
        (void) imap_pop(types->imap, type->type_id);
        return -1;
    }

    if (type->type_id >= types->next_id)
        types->next_id = type->type_id + 1;

    return 0;
}

/*
 * Do not use this function directly; Call ti_type_del(..) so
 * existing things using this type will be converted to objects and
 * restrictions will be removed.
 */
void ti_types_del(ti_types_t * types, ti_type_t * type)
{
    assert(!type->refcount);
    (void) imap_pop(types->imap, type->type_id);
    (void) smap_pop(types->smap, type->name);
}

typedef struct
{
    uint16_t id;
    ti_raw_t * nname;
} types__ren_t;

int types__spec_flags_pos(const unsigned char * x)
{
    int i = 0;
    while (x[i] == '^' ||
           x[i] == '&' ||
           x[i] == '-' ||
           x[i] == '+' ||
           x[i] == '*')
       i++;
    return i;
}

static int types__ren_member_cb(ti_type_t * type, ti_member_t * member)
{
    for (vec_each(type->fields, ti_field_t, field))
    {
        if (field->condition.none &&
                field->condition.none->dval == (ti_val_t *) member)
        {
            /* enum with default value rename */
            int flags_pos = types__spec_flags_pos(field->spec_raw->data);
            ti_member_t * member = \
                (ti_member_t *) field->condition.none->dval;
            ti_raw_t * spec_raw = ti_str_from_fmt(
                    "%.*s%s{%s}%s",
                    flags_pos,
                    (const char *) field->spec_raw->data,
                    member->enum_->name,
                    member->name->str,
                    (field->spec & TI_SPEC_NILLABLE) ? "?": "");
            if (!spec_raw)
                return -1;

            ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
            field->spec_raw = spec_raw;
        }
    }
    return 0;
}

static int types__ren_cb(ti_type_t * type, types__ren_t * w)
{
    for (vec_each(type->fields, ti_field_t, field))
    {
        if ((field->spec & TI_SPEC_MASK_NILLABLE) == w->id)
        {
            int flags_pos = types__spec_flags_pos(field->spec_raw->data);
            if (ti_spec_is_enum(field->spec) && field->condition.none)
            {
                /* enum with default value rename */
                ti_member_t * member = \
                    (ti_member_t *) field->condition.none->dval;
                ti_raw_t * spec_raw = ti_str_from_fmt(
                        "%.*s%.*s{%.*s}%s",
                        flags_pos,
                        (const char *) field->spec_raw->data,
                        w->nname->n,
                        (const char *) w->nname->data,
                        member->name->n, member->name->str,
                        (field->spec & TI_SPEC_NILLABLE) ? "?": "");
                if (!spec_raw)
                    return -1;

                ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
                field->spec_raw = spec_raw;
            }
            else if (field->spec == w->id && !flags_pos)
            {
                ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
                field->spec_raw = w->nname;
                ti_incref(field->spec_raw);
            }
            else
            {
                ti_raw_t * spec_raw = ti_str_from_fmt(
                        "%.*s%.*s%s",
                        flags_pos,
                        (const char *) field->spec_raw->data,
                        w->nname->n,
                        (const char *) w->nname->data,
                        (field->spec & TI_SPEC_NILLABLE) ? "?": "");
                if (!spec_raw)
                    return -1;

                ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
                field->spec_raw = spec_raw;
            }
        }
        else if ((field->nested_spec & TI_SPEC_MASK_NILLABLE) == w->id)
        {
            ti_raw_t * spec_raw;
            uint16_t spec = field->spec & TI_SPEC_MASK_NILLABLE;
            int flags_pos = types__spec_flags_pos(field->spec_raw->data);
            char * begin, end;
            switch (spec)
            {
            case TI_SPEC_ARR:
                begin = "[";
                end = ']';
                break;
            case TI_SPEC_SET:
                begin = "{";
                end = '}';
                break;
            case TI_SPEC_OBJECT:
                begin = "thing<";
                end = '>';
                break;
            default:
                assert(0);
                begin = "";
                end = '_';
            }
            spec_raw = ti_str_from_fmt(
                    "%.*s%s%.*s%s%c%s",
                    flags_pos,
                    (const char *) field->spec_raw->data,
                    begin,
                    w->nname->n,
                    (const char *) w->nname->data,
                    (field->nested_spec & TI_SPEC_NILLABLE) ? "?": "",
                    end,
                    (field->spec & TI_SPEC_NILLABLE) ? "?": "");

            if (!spec_raw)
                return -1;

            ti_val_unsafe_drop((ti_val_t *) field->spec_raw);
            field->spec_raw = spec_raw;
        }
    }
    return 0;
}

int ti_types_ren_spec(
        ti_types_t * types,
        uint16_t type_or_enum_id,
        ti_raw_t * nname)
{
    types__ren_t w = {
        .id = type_or_enum_id,
        .nname = nname,
    };

    return imap_walk(types->imap, (imap_cb) types__ren_cb, &w);
}

int ti_types_ren_member_spec(ti_types_t * types, ti_member_t * member)
{
    return imap_walk(types->imap, (imap_cb) types__ren_member_cb, member);
}

uint16_t ti_types_get_new_id(ti_types_t * types, ti_raw_t * rname, ex_t * e)
{
    uintptr_t utype;
    void * ptype;

    assert(rname->n <= TI_NAME_MAX);

    ptype = smap_popn(types->removed, (const char *) rname->data, rname->n);
    if (!ptype)
    {
        if (types->next_id == TI_SPEC_ANY)
        {
            ex_set(e, EX_MAX_QUOTA, "reached the maximum number of types");
        }
        return types->next_id;
    }

    utype = (uintptr_t) ptype;
    utype &= TI_TYPES_RM_MASK;

    assert(utype < types->next_id);
    assert(ti_types_by_id(types, utype) == NULL);

    return (uint16_t) utype;
}

typedef struct
{
    ti_varr_t * varr;
    _Bool with_definition;
} types__info_cb;

static int types__pack_type(ti_type_t * type, types__info_cb * w)
{
    ti_val_t * mpinfo = ti_type_as_mpval(type, w->with_definition);
    if (!mpinfo)
        return -1;
    VEC_push(w->varr->vec, mpinfo);
    return 0;
}

ti_varr_t * ti_types_info(ti_types_t * types, _Bool with_definition)
{
    ti_varr_t * varr = ti_varr_create(types->imap->n);
    if (!varr)
        return NULL;

    types__info_cb info_cb = {
            .varr = varr,
            .with_definition = with_definition,
    };

    if (smap_values(types->smap, (smap_val_cb) types__pack_type, &info_cb))
    {
        ti_val_unsafe_drop((ti_val_t *) varr);
        return NULL;
    }

    return varr;
}

static inline int types__to_pk_cb(ti_type_t * type, msgpack_packer * pk)
{
    return ti_type_to_pk(type, pk, false);
}

int ti_types_to_pk(ti_types_t * types, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, types->imap->n) ||
        imap_walk(types->imap, (imap_cb) types__to_pk_cb, pk)
    );
}
