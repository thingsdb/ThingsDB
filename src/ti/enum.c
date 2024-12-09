/*
 * ti/enum.c
 */
#include <doc.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/member.h>
#include <ti/method.h>
#include <ti/names.h>
#include <ti/opr.h>
#include <ti/raw.inline.h>
#include <ti/thing.inline.h>
#include <ti/val.inline.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/vec.h>
#include <util/logger.h>

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
    enum_->members = vec_new(2);;
    enum_->methods = vec_new(0);
    enum_->created_at = created_at;
    enum_->modified_at = modified_at;

    if (!enum_->name || !enum_->rname || !enum_->smap || !enum_->methods ||
        !enum_->members)
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
    vec_destroy(enum_->methods, (vec_destroy_cb) ti_method_destroy);
    vec_destroy(enum_->members, (vec_destroy_cb) ti_member_remove);
    if (enum_->rname)
        ti_val_unsafe_drop((ti_val_t *) enum_->rname);
    free(enum_->name);
    free(enum_);
}

/*
 * TODO (COMPAT) For compatibility with ThingsDB < v1.7.0
 */
int ti_enum_create_placeholders(ti_enum_t * enum_, size_t n, ex_t * e)
{
    if (ti_enum_prealloc(enum_, n, e))
        return e->nr;

    enum_->enum_tp = TI_ENUM_INT;

    while (n--)
        if (!ti_member_placeholder(enum_, e))
            break;
    return e->nr;
}

int ti_enum_prealloc(ti_enum_t * enum_, size_t sz, ex_t * e)
{
    if (!sz)
        ex_set(e, EX_VALUE_ERROR, "cannot create an empty enum type");
    else if (vec_make(&enum_->members, sz))
        ex_set_mem(e);

    return e->nr;
}

int ti_enum_prealloc_methods(ti_enum_t * enum_, size_t sz, ex_t * e)
{
    if (vec_make(&enum_->methods, sz))
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

static int enum__check_val(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
{
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
    return enum__check_val(enum_, val, e);
}

int ti_enum_members_check(ti_enum_t * enum_, ex_t * e)
{
    size_t i;
    size_t n = 0;
    size_t m = enum_->members->n;
    if (!m)
    {
        ex_set(e, EX_BAD_DATA, "no members");
        return e->nr;
    }
    for (vec_each(enum_->members, ti_member_t, member), n++)
    {
        if (!member->val)
        {
            ex_set(e, EX_BAD_DATA, "member value not set");
            return e->nr;
        }

        if (enum__check_val(enum_, member->val, e))
            return e->nr;

        for (i = n+1; i < m; i++)
        {
            ti_member_t * other = enum_->members->data[i];
            if (other->name == member->name)
            {
                ex_set(e, EX_BAD_DATA, "member names must be unique");
                return e->nr;
            }
        }
    }
    return 0;
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

    swap = vec_get(enum_->members, member->idx);
    if (swap)
        swap->idx = member->idx;

    (void) smap_pop(enum_->smap, member->name->str);
}

int ti_enum_add_method(
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_closure_t * closure,
        ex_t * e)
{
    ti_method_t * method;

    if (ti_enum_member_by_strn(enum_, name->str, name->n) ||
        ti_enum_get_method(enum_, name))
    {
        ex_set(e, EX_VALUE_ERROR,
            "member or method `%s` already exists on enum `%s`"DOC_T_TYPED,
            name->str,
            enum_->name);
        return e->nr;
    }

    method = ti_method_create(name, closure);
    if (!method)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (vec_push(&enum_->methods, method))
    {
        ex_set_mem(e);
        ti_method_destroy(method);
    }

    return e->nr;
}

void ti_enum_remove_method(ti_enum_t * enum_, ti_name_t * name)
{
    size_t idx = 0;
    for (vec_each(enum_->methods, ti_method_t, m), ++idx)
    {
        if (m->name == name)
        {
            ti_method_destroy(vec_swap_remove(enum_->methods, idx));
            return;
        }
    }
}

typedef struct
{
    ti_enum_t * enum_;
    ex_t * e;
} enum__init_t;

static int enum__init_cb(ti_item_t * item, enum__init_t * w)
{
    if (!ti_raw_is_name(item->key))
        ex_set(w->e, EX_VALUE_ERROR,
                "enum member must follow the naming rules"DOC_NAMES);
    else if (ti_val_is_closure(item->val))
        (void) ti_enum_add_method(
                w->enum_,
                (ti_name_t *) item->key,
                (ti_closure_t *) item->val,
                w->e);
    else
        (void) ti_member_create(
                w->enum_,
                (ti_name_t *) item->key,
                item->val,
                w->e);
    return w->e->nr;
}

static int enum__init_thing_o(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    if (ti_thing_is_dict(thing))
    {
        enum__init_t w = {
                .enum_ = enum_,
                .e = e,
        };
        return smap_values(thing->items.smap, (smap_val_cb) enum__init_cb, &w);
    }
    for (vec_each(thing->items.vec, ti_prop_t, prop))
    {
        if (ti_val_is_closure(prop->val))
        {
            if (ti_enum_add_method(
                    enum_,
                    prop->name,
                    (ti_closure_t *) prop->val,
                    e))
                return e->nr;
        }
        else if (!ti_member_create(enum_, prop->name, prop->val, e))
            return e->nr;
    }
    return 0;
}

static int enum__init_thing_t(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    ti_name_t * name;
    ti_val_t * val;
    for (thing_t_each(thing, name, val))
    {
        if (ti_val_is_closure(val))
        {
            if (ti_enum_add_method(
                    enum_,
                    name,
                    (ti_closure_t *) val,
                    e))
                return e->nr;
        }
        else  if (!ti_member_create(enum_, name, val, e))
            return e->nr;
    }
    return e->nr;
}

int ti_enum_init_from_thing(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    if (ti_enum_prealloc(enum_, ti_thing_n(thing), e))
        return e->nr;

    return ti_thing_is_object(thing)
        ? enum__init_thing_o(enum_, thing, e)
        : enum__init_thing_t(enum_, thing, e);
}

int ti_enum_set_members_from_vup(ti_enum_t * enum_, ti_vup_t * vup, ex_t * e)
{
    ti_name_t * name;
    ti_val_t * val = NULL;
    mp_obj_t obj, mp_name;
    ti_member_t * member;

    if (mp_next(vup->up, &obj) != MP_ARR)
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking members for enum `%s`;"
                "expecting the members as an array",
                enum_->name);
        return e->nr;
    }

    if (!obj.via.sz || obj.via.sz != enum_->members->n)
    {
        ex_set(e, EX_BAD_DATA,
                "the number of members for enum `%s` does not match "
                "the array size",
                enum_->name);
        return e->nr;
    }

    member = vec_first(enum_->members);
    if (ti_val_is_nil(member->val))
    {
        /* this is a sloppy test. before v1.7.0 members where stored as int,
         * now they are filled using nil */
        size_t i;
        for (i = obj.via.sz; i--;)
        {
            if (mp_next(vup->up, &obj) != MP_ARR || obj.via.sz != 2 ||
                mp_next(vup->up, &mp_name) != MP_STR)
            {
                ex_set(e, EX_BAD_DATA,
                        "failed unpacking members for enum `%s`;"
                        "expecting an array with two values",
                        enum_->name);
                return e->nr;
            }

            member = ti_enum_member_by_strn(
                enum_,
                mp_name.via.str.data,
                mp_name.via.str.n);
            if (!member || !ti_val_is_nil(member->val))
            {
                ex_set(e, EX_BAD_DATA,
                        "failed unpacking members for enum `%s`;"
                        "unable to find member",
                        enum_->name);
                return e->nr;
            }

            val = ti_val_from_vup_e(vup, e);
            if (!val)
                return e->nr;

            ti_decref(member->val);
            member->val = val;
        }
    }
    else
    {
        /* TODO (COMPAT) For compatibility with ThingsDB < v 1.7.0 */
        for (vec_each(enum_->members, ti_member_t, member))
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

            ti_name_drop(member->name);
            ti_val_drop(member->val);

            member->name = name;
            member->val = val;
        }
    }

    /* set enum type based on the last value */
    if (ti_enum_set_enum_tp(enum_, val, e))
        return e->nr;

    /* sanity check and return */
    return ti_enum_members_check(enum_, e);

failed:
    if (!e->nr)
        ex_set_mem(e);

    ti_name_drop(name);
    return e->nr;
}


int ti_enum_init_members_from_vup(ti_enum_t * enum_, ti_vup_t * vup, ex_t * e)
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

int ti_enum_init_methods_from_vup(ti_enum_t * enum_, ti_vup_t * vup, ex_t * e)
{
    ti_name_t * name;
    ti_val_t * val;
    mp_obj_t obj, mp_name;
    size_t i;

    if (mp_skip(vup->up) != MP_STR || mp_next(vup->up, &obj) != MP_ARR)
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking methods for enum `%s`;"
                "expecting the methods as an array",
                enum_->name);
        return e->nr;
    }

    if (ti_enum_prealloc_methods(enum_, obj.via.sz, e))
        return e->nr;

    for (i = obj.via.sz; i--;)
    {
        val = NULL;

        if (mp_next(vup->up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(vup->up, &mp_name) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking methods for enum `%s`;"
                    "expecting an array with two values",
                    enum_->name);
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_name.via.str.data, mp_name.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking methods for enum `%s`;"
                    "methods names must follow the naming rules"DOC_NAMES,
                    enum_->name);
            return e->nr;
        }

        name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
        if (!name)
            goto failed;

        val = ti_val_from_vup_e(vup, e);
        if (!val)
            goto failed;

        if (!ti_val_is_closure(val))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking methods for enum `%s`;"
                    "methods must be type closure",
                    enum_->name);
            goto failed;
        }

        if (ti_enum_add_method(enum_, name, (ti_closure_t *) val, e))
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
int ti_enum_members_to_client_pk(
        ti_enum_t * enum_,
        ti_vp_t * vp,
        int deep,
        int flags)
{
    if (msgpack_pack_array(&vp->pk, enum_->members->n))
        return -1;

    for (vec_each(enum_->members, ti_member_t, member))
    {
        if (msgpack_pack_array(&vp->pk, 2) ||
            mp_pack_strn(&vp->pk, member->name->str, member->name->n) ||
            ti_val_to_client_pk(member->val, vp, deep, flags))
            return -1;
    }

    return 0;
}

int ti_enum_members_to_store_pk(ti_enum_t * enum_, msgpack_packer * pk)
{
    if (msgpack_pack_array(pk, enum_->members->n))
        return -1;

    for (vec_each(enum_->members, ti_member_t, member))
    {
        if (msgpack_pack_array(pk, 2) ||
            mp_pack_strn(pk, member->name->str, member->name->n) ||
            ti_val_to_store_pk(member->val, pk))
            return -1;
    }

    return 0;
}

int ti_enum_methods_to_store_pk(ti_enum_t * enum_, msgpack_packer * pk)
{
    if (msgpack_pack_array(pk, enum_->methods->n))
        return -1;

    for (vec_each(enum_->methods, ti_method_t, method))
    {
        if (msgpack_pack_array(pk, 2) ||
            mp_pack_strn(pk, method->name->str, method->name->n) ||
            ti_val_to_store_pk((ti_val_t *) method->closure, pk))
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
    case TI_VAL_DATETIME:
    case TI_VAL_MPDATA:
    case TI_VAL_REGEX:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_FUTURE:
    case TI_VAL_MODULE:
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
    ti_vp_t vp;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&vp.pk, &buffer, msgpack_sbuffer_write);

    if (ti_enum_to_pk(enum_, &vp))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;
}

static int enum__methods_info_to_pk(ti_enum_t * enum_, msgpack_packer * pk)
{
    if (msgpack_pack_map(pk, enum_->methods->n))
        return -1;

    for (vec_each(enum_->methods, ti_method_t, method))
    {
        ti_raw_t * doc = ti_method_doc(method);
        ti_raw_t * def;

        if (mp_pack_strn(pk, method->name->str, method->name->n) ||

            msgpack_pack_map(pk, 4) ||

            mp_pack_str(pk, "doc") ||
            mp_pack_strn(pk, doc->data, doc->n) ||

            ((def = ti_method_def(method)) && (
                mp_pack_str(pk, "definition") ||
                mp_pack_strn(pk, def->data, def->n)
            )) ||

            mp_pack_str(pk, "with_side_effects") ||
            mp_pack_bool(pk, method->closure->flags & TI_CLOSURE_FLAG_WSE) ||

            mp_pack_str(pk, "arguments") ||
            msgpack_pack_array(pk, method->closure->vars->n))
            return -1;

        for (vec_each(method->closure->vars, ti_prop_t, prop))
            if (mp_pack_str(pk, prop->name->str))
                return -1;
    }
    return 0;
}

int ti_enum_to_pk(ti_enum_t * enum_, ti_vp_t * vp)
{
    ti_member_t * def = VEC_first(enum_->members);
    return (
        msgpack_pack_map(&vp->pk, 7) ||
        mp_pack_str(&vp->pk, "enum_id") ||
        msgpack_pack_uint16(&vp->pk, enum_->enum_id) ||

        mp_pack_str(&vp->pk, "name") ||
        mp_pack_strn(&vp->pk, enum_->rname->data, enum_->rname->n) ||

        mp_pack_str(&vp->pk, "default") ||
        mp_pack_strn(&vp->pk, def->name->str, def->name->n) ||

        mp_pack_str(&vp->pk, "created_at") ||
        msgpack_pack_uint64(&vp->pk, enum_->created_at) ||

        mp_pack_str(&vp->pk, "modified_at") ||
        (enum_->modified_at
            ? msgpack_pack_uint64(&vp->pk, enum_->modified_at)
            : msgpack_pack_nil(&vp->pk)) ||

        mp_pack_str(&vp->pk, "members") ||
        ti_enum_members_to_client_pk(enum_, vp, 0, 0)  /* only ID; one level */ ||

        mp_pack_str(&vp->pk, "methods") ||
        enum__methods_info_to_pk(enum_, &vp->pk)
    );
}

