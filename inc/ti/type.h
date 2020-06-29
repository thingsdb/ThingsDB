/*
 * ti/type.h
 */
#ifndef TI_TYPE_H_
#define TI_TYPE_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/closure.t.h>
#include <ti/name.t.h>
#include <ti/thing.t.h>
#include <ti/type.t.h>
#include <ti/types.t.h>
#include <ti/val.t.h>
#include <util/mpack.h>
#include <util/vec.h>

ti_type_t * ti_type_create(
        ti_types_t * types,
        uint16_t type_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at);
void ti_type_drop(ti_type_t * type);
void ti_type_del(ti_type_t * type);
void ti_type_destroy(ti_type_t * type);
void ti_type_map_cleanup(ti_type_t * type);
size_t ti_type_fields_approx_pack_sz(ti_type_t * type);
int ti_type_init_from_thing(ti_type_t * type, ti_thing_t * thing, ex_t * e);
int ti_type_init_from_unp(
        ti_type_t * type,
        mp_unp_t * up,
        ex_t * e,
        _Bool with_methods);
int ti_type_fields_to_pk(ti_type_t * type, msgpack_packer * pk);
ti_val_t * ti_type_as_mpval(ti_type_t * type, _Bool with_definition);
vec_t * ti_type_map(ti_type_t * to_type, ti_type_t * from_type);
ti_val_t * ti_type_dval(ti_type_t * type);
ti_thing_t * ti_type_from_thing(ti_type_t * type, ti_thing_t * from, ex_t * e);
int ti_type_add_method(
        ti_type_t * type,
        ti_name_t * name,
        ti_closure_t * closure,
        ex_t * e);
void ti_type_remove_method(ti_type_t * type, ti_name_t * name);
int ti_type_methods_to_pk(ti_type_t * type, msgpack_packer * pk);
int ti_type_methods_info_to_pk(
        ti_type_t * type,
        msgpack_packer * pk,
        _Bool with_definition);


static inline int ti_type_try_lock(ti_type_t * type, ex_t * e)
{
    if (type->flags & TI_TYPE_FLAG_LOCK)
    {
        ex_set(e, EX_OPERATION_ERROR,
            "cannot change type `%s` while the type is being used",
            type->name);
        return -1;
    }
    return (type->flags |= TI_TYPE_FLAG_LOCK) & 0;
}

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

static inline int ti_type_to_pk(
        ti_type_t * type,
        msgpack_packer * pk,
        _Bool with_definition)
{
    return (
        msgpack_pack_map(pk, 6) ||
        mp_pack_str(pk, "type_id") ||
        msgpack_pack_uint16(pk, type->type_id) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, type->rname->data, type->rname->n) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, type->created_at) ||

        mp_pack_str(pk, "modified_at") ||
        (type->modified_at
            ? msgpack_pack_uint64(pk, type->modified_at)
            : msgpack_pack_nil(pk)) ||

        mp_pack_str(pk, "fields") ||
        ti_type_fields_to_pk(type, pk) ||

        mp_pack_str(pk, "methods") ||
        ti_type_methods_info_to_pk(type, pk, with_definition)
    );
}

#endif  /* TI_TYPE_H_ */

