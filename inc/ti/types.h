/*
 * ti/types.h
 */
#ifndef TI_TYPES_H_
#define TI_TYPES_H_

#include <ex.h>
#include <stdint.h>
#include <ti/collection.t.h>
#include <ti/raw.t.h>
#include <ti/type.t.h>
#include <ti/types.t.h>
#include <ti/varr.t.h>
#include <util/mpack.h>

ti_types_t * ti_types_create(ti_collection_t * collection);
void ti_types_destroy(ti_types_t * types);
int ti_types_add(ti_types_t * types, ti_type_t * type);
void ti_types_del(ti_types_t * types, ti_type_t * type);
int ti_types_ren_spec(
        ti_types_t * types,
        uint16_t type_or_enum_id,
        ti_raw_t * nname);
int ti_types_ren_member_spec(ti_types_t * types, ti_member_t * member);
uint16_t ti_types_get_new_id(ti_types_t * types, ti_raw_t * rname, ex_t * e);
ti_varr_t * ti_types_info(ti_types_t * types, _Bool with_definition);
int ti_types_to_pk(ti_types_t * types, msgpack_packer * pk);

#endif /* TI_TYPES_H_ */
