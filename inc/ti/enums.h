/*
 * ti/enums.h
 */
#ifndef TI_ENUMS_H_
#define TI_ENUMS_H_

typedef struct ti_enums_s ti_enums_t;

#include <ex.h>
#include <stdint.h>
#include <ti/collection.h>
#include <ti/raw.h>
#include <ti/enum.h>
#include <ti/varr.h>
#include <util/imap.h>
#include <util/mpack.h>
#include <util/smap.h>

ti_enums_t * ti_enums_create(ti_collection_t * collection);
void ti_enums_destroy(ti_enums_t * enums);
int ti_enums_add(ti_enums_t * enums, ti_enum_t * enum_);
void ti_enums_del(ti_enums_t * enums, ti_enum_t * enum_);
uint16_t ti_enums_get_new_id(ti_enums_t * enums, ti_raw_t * rname, ex_t * e);
ti_varr_t * ti_enums_info(ti_enums_t * enums);
int ti_enums_to_pk(ti_enums_t * enums, msgpack_packer * pk);

struct ti_enums_s
{
    imap_t * imap;
    smap_t * smap;
    ti_collection_t * collection;
};

#endif /* TI_ENUMS_H_ */
