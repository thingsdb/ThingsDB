/*
 * ti/enum.c
 */
#include <ti/enum.h>
#include <doc.h>

ti_enum_t * ti_enum_create(
        ti_enums_t * enums,
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

    if (!enum_->name || !enum_->rname || !enum_->smap || !enum_->members ||
            ti_enums_add(enums, enum_))
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

    vec_destroy(enum_->members, NULL);
    smap_destroy(enum_->smap, (vec_destroy_cb) ti_val_drop);
    ti_val_drop((ti_val_t *) enum_->rname);
    free(enum_->name);
    free(enum_);
}

int ti_enum_prealloc(ti_enum_t * enum_, size_t sz, ex_t * e)
{
    if (!sz)
    {
        ex_set(e, EX_VALUE_ERROR, "cannot create an empty enum type");
        return e->nr;
    }
    enum_->members = vec_reserve(&enum_->members, sz);
    if (!enum_->members)
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
                "enum `%s` cannot be created; "
                "enumerators cannot be created for values of type `%s`"
                DOC_T_ENUM,
                enum_->name,
                ti_val_str(val));
    }
    return e->nr;
}

int ti_enum_check_val(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
{
    ti_member_t * member = ti_enum_val_by_val(enum_, val);

    if (member)
    {
        ex_set(e, EX_VALUE_ERROR,
                "cannot create enum member `%s` on `%s`; "
                "each value must be unique"DOC_T_ENUM,
                member->name->str,
                enum_->name);
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
    if (enum_->members->n == UINT16_MAX)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of enum members has been reached");
        return e->nr;
    }

    if (vec_push(enum_->members, member))
        ex_set_mem(e);
    else if (smap_add(enum_->smap, member->name->str, member))
    {
        vec_pop(enum_->members);
        ex_set_mem(e);
    }

    return e->nr;
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

/* adds a map with key/value pairs */
int ti_enum_members_to_pk(ti_enum_t * enum_, msgpack_packer * pk)
{
    if (msgpack_pack_array(pk, enum_->members->n))
        return -1;

    for (vec_each(enum_->members, ti_member_t, member))
    {
        if (msgpack_pack_array(pk, 2) ||
            mp_pack_strn(pk, member->name->str, member->name->n) ||
            ti_val_to_pk(member->val, pk, ))
            return -1;
    }

    return 0;
}

ti_member_t * ti_enum_val_by_val_e(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
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
    case TI_VAL_ENUM:
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
            ti_enum_str_tp(enum_),
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

