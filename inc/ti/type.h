/*
 * ti/type.h
 */
#ifndef TI_TYPE_H_
#define TI_TYPE_H_

#define TI_TYPE_NAME_MAX 1024

typedef struct ti_type_s ti_type_t;

#include <inttypes.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/types.h>
#include <util/vec.h>
#include <qpack.h>
#include <ex.h>

ti_type_t * ti_type_create(
        ti_types_t * types,
        uint16_t type_id,
        const char * name,
        size_t n);
void ti_type_drop(ti_type_t * type);
void ti_type_destroy(ti_type_t * type);
size_t ti_type_approx_pack_sz(ti_type_t * type);
_Bool ti_type_is_valid_strn(const char * str, size_t n);
int ti_type_init_from_thing(ti_type_t * type, ti_thing_t * thing, ex_t * e);
int ti_type_init_from_unp(ti_type_t * type, qp_unpacker_t * unp, ex_t * e);
int ti_type_fields_to_packer(ti_type_t * type, qp_packer_t ** packer);
ti_val_t * ti_type_info_as_qpval(ti_type_t * type);
vec_t * ti_type_map(ti_type_t * to_type, ti_type_t * from_type, ex_t * e);

struct ti_type_s
{
    uint32_t refcount;      /* this counter will be incremented and decremented
                               by other types; the type may only be removed
                               when this counter is equal to zero otherwise
                               other types will break; self references are not
                               included in this couter */
    uint16_t type_id;       /* type id */
    uint16_t name_n;        /* name length (restricted to TI_TYPE_NAME_MAX) */
    char * name;            /* name (null terminated) */
    ti_types_t * types;
    vec_t * dependencies;   /* ti_type_t; contains type where this type is
                               depended on. type may be more than one inside
                               this vector but a self dependency is not
                               included */
    vec_t * fields;         /* ti_field_t */
    imap_t * t_mappings;    /* from_type_id / vec_t * with ti_field_t */
};

#endif  /* TI_TYPE_H_ */

