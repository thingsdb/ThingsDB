/*
 * ti/enum.c
 */
#include <doc.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/member.h>
#include <ti/names.h>
#include <ti/opr.h>
#include <ti/raw.inline.h>
#include <ti/thing.inline.h>
#include <ti/val.inline.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/vec.h>

ti_enum_t * ti_enum_create(
        uint16_t enum_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at)
{
    ti_enum_t * enum_ = malloc(sizeof(ti_enum_t));
    if (!enum_)
        return NULL;

    enum_->refcount = 0;
    enum_->enum_id = enum_id;
    enum_->flags = 0;
    enum_->name = strndup(name, name_n);
    enum_->rname = ti_str_create(name, name_n);
    enum_->smap = smap_create();
    enum_->members = NULL;
    enum_->created_at = created_at;
    enum_->modified_at = modified_at;

    if (!enum_->name || !enum_->rname || !enum_->smap)
    {
        ti_enum_destroy(enum_);
        return NULL;
    }

    return enum_;
}

void ti_enum_destroy(ti_enum_t * enum_)
{
    if (!enum_)
        return;

    smap_destroy(enum_->smap, NULL);
    vec_destroy(enum_->members, (vec_destroy_cb) ti_member_remove);
    ti_val_drop((ti_val_t *) enum_->rname);
    free(enum_->name);
    free(enum_);
}

int ti_enum_prealloc(ti_enum_t * enum_, size_t sz, ex_t * e)
{
    if (!sz)
        ex_set(e, EX_VALUE_ERROR, "cannot create an empty enum type");
    else if (vec_reserve(&enum_->members, sz))
        ex_set_mem(e);

    return e->nr;
}

int ti_enum_set_enum_tp(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
{
    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_INT:            enum_->enum_tp = TI_ENUM_INT;    break;
    case TI_VAL_FLOAT:          enum_->enum_tp = TI_ENUM_FLOAT;  break;
    case TI_VAL_NAME:
    case TI_VAL_STR:            enum_->enum_tp = TI_ENUM_STR;    break;
    case TI_VAL_BYTES:          enum_->enum_tp = TI_ENUM_BYTES;  break;
    case TI_VAL_THING:          enum_->enum_tp = TI_ENUM_THING;  break;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "failed to create enum type `%s`; "
                "enumerators cannot be created for values with type `%s`"
                DOC_T_ENUM,
                enum_->name,
                ti_val_str(val));
    }
    return e->nr;
}

int ti_enum_check_val(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
{
    ti_member_t * member = ti_enum_member_by_val(enum_, val);

    if (member)
    {
        ex_set(e, EX_VALUE_ERROR,
                "enum values must be unique; the given value is already "
                "used by `%s{%s}`"DOC_T_ENUM,
                enum_->name,
                member->name->str);
        return e->nr;
    }

    switch((ti_enum_enum) enum_->enum_tp)
    {
    case TI_ENUM_INT:
        if (ti_val_is_int(val))
            return 0;
        break;
    case TI_ENUM_FLOAT:
        if (ti_val_is_float(val))
            return 0;
        break;
    case TI_ENUM_STR:
        if (ti_val_is_str(val))
            return 0;
        break;
    case TI_ENUM_BYTES:
        if (ti_val_is_bytes(val))
            return 0;
        break;
    case TI_ENUM_THING:
        if (ti_val_is_thing(val))
            return 0;
        break;
    }

    ex_set(e, EX_TYPE_ERROR,
            "enumerators must not contain members of mixed types; "
            "got both type `%s` and `%s` for enum `%s`"DOC_T_ENUM,
            ti_enum_tp_str(enum_),
            ti_val_str(val),
            enum_->name);

    return e->nr;
}

/*
 * this does not increment with a reference
 */
int ti_enum_add_member(ti_enum_t * enum_, ti_member_t * member, ex_t * e)
{
    uint32_t n = enum_->members->n;

    if (n == UINT16_MAX)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of enum members has been reached");
        return e->nr;
    }

    member->idx = n;

    if (vec_push(&enum_->members, member))
        ex_set_mem(e);
    else if (smap_add(enum_->smap, member->name->str, member))
    {
        vec_pop(enum_->members);
        ex_set_mem(e);
    }

    return e->nr;
}

void ti_enum_del_member(ti_enum_t * enum_, ti_member_t * member)
{
    ti_member_t * swap;

    (void) vec_swap_remove(enum_->members, member->idx);

    swap = vec_get_or_null(enum_->members, member->idx);
    if (swap)
        swap->idx = member->idx;

    (void) smap_pop(enum_->smap, member->name->str);
}

static int enum__init_thing_o(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    for (vec_each(thing->items, ti_prop_t, prop))
        if (!ti_member_create(enum_, prop->name, prop->val, e))
            return e->nr;

    return e->nr;
}

static int enum__init_thing_t(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    ti_name_t * name;
    ti_val_t * val;
    for (thing_t_each(thing, name, val))
        if (!ti_member_create(enum_, name, val, e))
            return e->nr;

    return e->nr;
}

int ti_enum_init_from_thing(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    if (ti_enum_prealloc(enum_, thing->items->n, e))
        return e->nr;

    return ti_thing_is_object(thing)
        ? enum__init_thing_o(enum_, thing, e)
        : enum__init_thing_t(enum_, thing, e);
}

int ti_enum_init_from_vup(ti_enum_t * enum_, ti_vup_t * vup, ex_t * e)
{
    ti_name_t * name;
    ti_val_t * val;
    mp_obj_t obj, mp_name;
    size_t i;

    if (mp_next(vup->up, &obj) != MP_ARR)
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking members for enum `%s`;"
                "expecting the members as an array",
                enum_->name);
        return e->nr;
    }

    if (ti_enum_prealloc(enum_, obj.via.sz, e))
        return e->nr;

    for (i = obj.via.sz; i--;)
    {
        val = NULL;

        if (mp_next(vup->up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(vup->up, &mp_name) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking members for enum `%s`;"
                    "expecting an array with two values",
                    enum_->name);
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_name.via.str.data, mp_name.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking members for enum `%s`;"
                    "member names must follow the naming rules"DOC_NAMES,
                    enum_->name);
            return e->nr;
        }

        name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
        if (!name)
            goto failed;

        val = ti_val_from_vup_e(vup, e);
        if (!val)
            goto failed;

        if (!ti_member_create(enum_, name, val, e))
            goto failed;

        ti_decref(name);
        ti_decref(val);
    }

    return e->nr;

failed:
    if (!e->nr)
        ex_set_mem(e);

    ti_name_drop(name);
    ti_val_drop(val);

    return e->nr;
}

/* adds a map with key/value pairs */
int ti_enum_members_to_pk(ti_enum_t * enum_, msgpack_packer * pk, int options)
{
    if (msgpack_pack_array(pk, enum_->members->n))
        return -1;

    for (vec_each(enum_->members, ti_member_t, member))
    {
        if (msgpack_pack_array(pk, 2) ||
            mp_pack_strn(pk, member->name->str, member->name->n) ||
            ti_val_to_pk(member->val, pk, options))
            return -1;
    }

    return 0;
}

ti_member_t * ti_enum_member_by_val(ti_enum_t * enum_, ti_val_t * val)
{
    for(vec_each(enum_->members, ti_member_t, member))
        if (ti_opr_eq(VMEMBER(member), val))
            return member;
    return NULL;
}

ti_member_t * ti_enum_member_by_val_e(
        ti_enum_t * enum_,
        ti_val_t * val,
        ex_t * e)
{
    for(vec_each(enum_->members, ti_member_t, member))
        if (ti_opr_eq(VMEMBER(member), val))
            return member;

    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_BOOL:
    case TI_VAL_MP:
    case TI_VAL_REGEX:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_TEMPLATE:
        break;

    case TI_VAL_INT:
        if (enum_->enum_tp != TI_ENUM_INT)
            break;
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member with value %"PRId64,
                enum_->name,
                VINT(val));
        return NULL;

    case TI_VAL_FLOAT:
        if (enum_->enum_tp != TI_ENUM_INT)
            break;
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member with value %f",
                enum_->name,
                VFLOAT(val));
        return NULL;

    case TI_VAL_NAME:
    case TI_VAL_STR:
        if (enum_->enum_tp != TI_ENUM_STR)
            break;
        else
        {
            size_t sz = ((ti_raw_t *) val)->n;
            sz = sz > 20 ? 20 : sz;
            ex_set(e, EX_LOOKUP_ERROR,
                    "enum `%s` has no member with value `%.*s`",
                    enum_->name,
                    (int) sz,
                    (char *) ((ti_raw_t *) val)->data);
            return NULL;
        }
    case TI_VAL_BYTES:
        if (enum_->enum_tp != TI_ENUM_STR)
            break;
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member with a value equal "
                "to the given bytes",
                enum_->name);
        return NULL;

    case TI_VAL_THING:
        if (enum_->enum_tp != TI_ENUM_THING)
            break;
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member with thing "TI_THING_ID" as value",
                enum_->name,
                ((ti_thing_t *) val)->id);
        return NULL;
    }

    ex_set(e, EX_TYPE_ERROR,
            "enum `%s` is expecting a value of type `%s` "
            "but got type `%s` instead",
            enum_->name,
            ti_enum_tp_str(enum_),
            ti_val_str(val));
    return NULL;
}


ti_val_t * ti_enum_as_mpval(ti_enum_t * enum_)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_enum_to_pk(enum_, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}

int ti_enum_to_pk(ti_enum_t * enum_, msgpack_packer * pk)
{
    ti_member_t * def = VEC_first(enum_->members);
    return (
        msgpack_pack_map(pk, 6) ||
        mp_pack_str(pk, "enum_id") ||
        msgpack_pack_uint16(pk, enum_->enum_id) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, enum_->rname->data, enum_->rname->n) ||

        mp_pack_str(pk, "default") ||
        mp_pack_strn(pk, def->name->str, def->name->n) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, enum_->created_at) ||

        mp_pack_str(pk, "modified_at") ||
        (enum_->modified_at
            ? msgpack_pack_uint64(pk, enum_->modified_at)
            : msgpack_pack_nil(pk)) ||

        mp_pack_str(pk, "members") ||
        ti_enum_members_to_pk(enum_, pk, 0)  /* only ID's for one level */
    );
}

