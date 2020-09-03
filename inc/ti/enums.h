/*
 * ti/enums.h
 */
#ifndef TI_ENUMS_H_
#define TI_ENUMS_H_

#include <ex.h>
#include <stdint.h>
#include <ti/collection.t.h>
#include <ti/enum.t.h>
#include <ti/enums.t.h>
#include <ti/varr.t.h>
#include <util/mpack.h>

ti_enums_t * ti_enums_create(ti_collection_t * collection);
void ti_enums_destroy(ti_enums_t * enums);
int ti_enums_add(ti_enums_t * enums, ti_enum_t * enum_);
int ti_enums_rename(ti_enums_t * enums, ti_enum_t * enum_, ti_raw_t * nname);
void ti_enums_del(ti_enums_t * enums, ti_enum_t * enum_);
uint16_t ti_enums_get_new_id(ti_enums_t * enums, ex_t * e);
ti_varr_t * ti_enums_info(ti_enums_t * enums);
int ti_enums_to_pk(ti_enums_t * enums, msgpack_packer * pk);

#endif /* TI_ENUMS_H_ */
