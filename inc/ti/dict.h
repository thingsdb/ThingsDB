/*
 * ti/dict.h
 */
#ifndef TI_DICT_H_
#define TI_DICT_H_

#include <ex.h>
#include <stdint.h>
#include <ti/val.t.h>
#include <ti/dict.t.h>
#include <util/vec.h>

typedef enum
{
    TI_DICT_KEY_INT,
    TI_DICT_KEY_STR,
    TI_DICT_KEY_UUID,
} ti_dict_enum_e;

typedef struct
{
    const char * str;
    size_t n;
} ti_dict_key_str_t;

typedef union
{
    int64_t id;
    uuid_t * uuid;
    ti_dict_key_str_t str;
} ti_dict_key_u;


typedef struct
{
    ti_dict_key_u via;
    ti_dict_enum_e tp;
} ti_dict_key_t;

typedef int (*ti_dict_cb)(ti_val_t * val, void * arg);
typedef int (*ti_dict_item_cb)(ti_val_t * key, ti_val_t * val, void * arg);
typedef int (*ti_dict_pair_cb)(ti_dict_key_t * key, ti_val_t * val, void * arg);

ti_dict_t * ti_dict_create(void);
void ti_dict_destroy(ti_dict_t * dict);
int ti_dict_to_client_pk(ti_dict_t * dict, ti_vp_t * vp, int deep, int flags);
int ti_dict_to_store_pk(ti_dict_t * dict, msgpack_packer * pk);
int ti_dict_to_list(ti_dict_t ** dict);
int ti_dict_to_tuple(ti_dict_t ** dict);
int ti_dict_copy(ti_dict_t ** dict, uint8_t deep);
int ti_dict_dup(ti_dict_t ** dict, uint8_t deep);
int ti_dict_walk(ti_dict_t * dict, ti_dict_cb cb, void * arg);
int ti_dict_items(ti_dict_t * dict, ti_dict_item_cb cb, void * arg);
int ti_dict_pairs(ti_dict_t * dict, ti_dict_pair_cb cb, void * arg);
int ti_dict_nested_spec_err(ti_dict_t * dict, ti_val_t * val, ex_t * e);
_Bool ti_dict_has_val(ti_dict_t * dict, ti_val_t * val);
ti_val_t * ti_dict_get(ti_dict_t * dict, ti_val_t * key)

#endif  /* TI_DICT_H_ */

