/*
 * ti/type.h
 */
#ifndef TI_TYPE_H_
#define TI_TYPE_H_

typedef struct ti_type_s ti_type_t;

enum
{
    TI_TYPE_FLAG_LOCK       =1<<0,
};

#include <ex.h>
#include <inttypes.h>
#include <ti/thing.h>
#include <ti/types.h>
#include <ti/val.h>
#include <util/vec.h>
#include <util/mpack.h>

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
_Bool ti_type_is_valid_strn(const char * str, size_t n);
int ti_type_init_from_thing(ti_type_t * type, ti_thing_t * thing, ex_t * e);
int ti_type_init_from_unp(ti_type_t * type, mp_unp_t * up, ex_t * e);
int ti_type_fields_to_pk(ti_type_t * type, msgpack_packer * pk);
ti_val_t * ti_type_as_mpval(ti_type_t * type);
vec_t * ti_type_map(ti_type_t * to_type, ti_type_t * from_type);

struct ti_type_s
{
    uint32_t refcount;      /* this counter will be incremented and decremented
                               by other types; the type may only be removed
                               when this counter is equal to zero otherwise
                               other types will break; self references are not
                               included in this counter */
    uint16_t type_id;       /* type id */
    uint8_t flags;          /* type flags */
    uint8_t pad0_;
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    uint64_t modified_at;   /* UNIX time-stamp in seconds */
    char * name;            /* name (null terminated) */
    char * wname;           /* wrapped name (null terminated) */
    ti_raw_t * rname;       /* name as raw type */
    ti_raw_t * rwname;      /* wrapped name as raw type */
    ti_types_t * types;
    vec_t * dependencies;   /* ti_type_t; contains type where this type is
                               depended on. type may be more than one inside
                               this vector but a self dependency is not
                               included, order is not important */
    vec_t * fields;         /* ti_field_t */
    imap_t * t_mappings;    /* from_type_id / vec_t * with ti_field_t */
};

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

static inline int ti_type_to_pk(ti_type_t * type, msgpack_packer * pk)
{
    return (
        msgpack_pack_map(pk, 5) ||
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
        ti_type_fields_to_pk(type, pk)
    );
}

#endif  /* TI_TYPE_H_ */

