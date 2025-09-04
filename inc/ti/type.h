/*
 * ti/type.h
 */
#ifndef TI_TYPE_H_
#define TI_TYPE_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/change.t.h>
#include <ti/closure.t.h>
#include <ti/method.t.h>
#include <ti/name.t.h>
#include <ti/map.h>
#include <ti/thing.t.h>
#include <ti/type.t.h>
#include <ti/types.t.h>
#include <ti/val.t.h>
#include <util/mpack.h>
#include <util/vec.h>

ti_type_t * ti_type_create(
        ti_types_t * types,
        uint16_t type_id,
        uint8_t flags,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at);
void ti_type_drop(ti_type_t * type);
void ti_type_del(ti_type_t * type, vec_t * vars);
void ti_type_destroy(ti_type_t * type);
void ti_type_map_cleanup(ti_type_t * type);
size_t ti_type_fields_approx_pack_sz(ti_type_t * type);
int ti_type_init_from_thing(ti_type_t * type, ti_thing_t * thing, ex_t * e);
int ti_type_init_from_unp(
        ti_type_t * type,
        mp_unp_t * up,
        ex_t * e,
        _Bool with_methods,
        _Bool with_wrap_only,
        _Bool with_hide_id);
int ti_type_fields_to_pk(ti_type_t * type, msgpack_packer * pk);
ti_val_t * ti_type_as_mpval(ti_type_t * type, _Bool with_definition);
ti_map_t * ti_type_map(ti_type_t * to_type, ti_type_t * from_type);
ti_val_t * ti_type_dval(ti_type_t * type);
int ti_type_convert(
        ti_type_t * type,
        ti_thing_t * thing,
        ti_change_t * change,
        ex_t * e);
ti_thing_t * ti_type_from_thing(ti_type_t * type, ti_thing_t * from, ex_t * e);
int ti_type_add_method(
        ti_type_t * type,
        ti_name_t * name,
        ti_closure_t * closure,
        ex_t * e);
void ti_type_remove_method(ti_type_t * type, ti_name_t * name);
int ti_type_set_idname(
        ti_type_t * type,
        const char * s,
        size_t n,
        ex_t * e);
int ti_type_methods_to_pk(ti_type_t * type, msgpack_packer * pk);
int ti_type_relations_to_pk(ti_type_t * type, msgpack_packer * pk);
int ti_type_methods_info_to_pk(
        ti_type_t * type,
        msgpack_packer * pk,
        _Bool with_definition);
int ti_type_required_by_non_wpo(ti_type_t * type, ex_t * e);
int ti_type_uses_wpo(ti_type_t * type, ex_t * e);
int ti_type_rename(ti_type_t * type, ti_raw_t * nname);

static inline int ti_type_use(ti_type_t * type, ex_t * e)
{
    if (type->flags & TI_TYPE_FLAG_LOCK)
    {
        ex_set(e, EX_OPERATION,
            "cannot use type `%s` while the type is locked",
            type->name);
        return e->nr;
    }
    return 0;
}

static inline int ti_type_try_lock(ti_type_t * type, ex_t * e)
{
    if (type->flags & TI_TYPE_FLAG_LOCK)
    {
        ex_set(e, EX_OPERATION,
            "cannot change type `%s` while the type is in use",
            type->name);
        return e->nr;
    }
    return (type->flags |= TI_TYPE_FLAG_LOCK) & 0;
}

/*
 * Returns `0` if a lock was already set and `1` if locked by this call.
 */
static inline int ti_type_ensure_lock(ti_type_t * type)
{
    return (type->flags & TI_TYPE_FLAG_LOCK)
            ? 0
            : !!(type->flags |= TI_TYPE_FLAG_LOCK);
}

static inline void ti_type_unlock(ti_type_t * type, int lock_was_set)
{
    if (lock_was_set)
        type->flags &= ~TI_TYPE_FLAG_LOCK;
}

static inline _Bool ti_type_is_wrap_only(ti_type_t * type)
{
    return type->flags & TI_TYPE_FLAG_WRAP_ONLY;
}

static inline _Bool ti_type_hide_id(ti_type_t * type)
{
    return type->flags & TI_TYPE_FLAG_HIDE_ID;
}

static inline int ti_type_to_pk(
        ti_type_t * type,
        msgpack_packer * pk,
        _Bool with_definition)
{
    return (
        msgpack_pack_map(pk, 9) ||
        mp_pack_str(pk, "type_id") ||
        msgpack_pack_uint16(pk, type->type_id) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, type->rname->data, type->rname->n) ||

        mp_pack_str(pk, "wrap_only") ||
        mp_pack_bool(pk, ti_type_is_wrap_only(type)) ||

        mp_pack_str(pk, "hide_id") ||
        mp_pack_bool(pk, ti_type_hide_id(type)) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, type->created_at) ||

        mp_pack_str(pk, "modified_at") ||
        (type->modified_at
            ? msgpack_pack_uint64(pk, type->modified_at)
            : msgpack_pack_nil(pk)) ||

        mp_pack_str(pk, "fields") ||
        ti_type_fields_to_pk(type, pk) ||

        mp_pack_str(pk, "methods") ||
        ti_type_methods_info_to_pk(type, pk, with_definition) ||

        mp_pack_str(pk, "relations") ||
        ti_type_relations_to_pk(type, pk)
    );
}

static inline void ti_type_set_wrap_only_mode(ti_type_t * type, _Bool wpo)
{
    if (wpo)
        type->flags |= TI_TYPE_FLAG_WRAP_ONLY;
    else
        type->flags &= ~TI_TYPE_FLAG_WRAP_ONLY;
}

static inline void ti_type_set_hide_id(ti_type_t * type, _Bool hid)
{
    if (hid)
        type->flags |= TI_TYPE_FLAG_HIDE_ID;
    else
        type->flags &= ~TI_TYPE_FLAG_HIDE_ID;
}


static inline int ti_type_wrap_only_e(ti_type_t * type, ex_t * e)
{
    if (ti_type_is_wrap_only(type))
        ex_set(e, EX_TYPE_ERROR,
                "type `%s` has wrap-only mode enabled", type->name);
    return e->nr;
}

static inline ti_method_t * ti_type_get_method(
        ti_type_t * type,
        ti_name_t * name)
{
    for (vec_each(type->methods, ti_method_t, method))
        if (method->name == name)
            return method;
    return NULL;
}

#endif  /* TI_TYPE_H_ */

