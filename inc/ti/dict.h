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

typedef int (*ti_dict_cb)(ti_val_t * key, ti_val_t * val, void * arg);

ti_dict_t * ti_dict_create(void);
ti_dict_t * ti_dict_cp(ti_dict_t * dict);
void ti_dict_destroy(ti_dict_t * dict);
int ti_dict_to_client_pk(ti_dict_t * dict, ti_vp_t * vp, int deep, int flags);
int ti_dict_to_store_pk(ti_dict_t * dict, msgpack_packer * pk);
int ti_dict_to_list(ti_dict_t ** dict);
int ti_dict_to_tuple(ti_dict_t ** dict);
int ti_dict_copy(ti_dict_t ** dict, uint8_t deep);
int ti_dict_dup(ti_dict_t ** dict, uint8_t deep);
_Bool ti__dict_eq(ti_dict_t * dicta, ti_dict_t * dictb);
_Bool ti_dict_has_val(ti_dict_t * dict, ti_val_t * val);
int ti_dict_nested_spec_err(ti_dict_t * dict, ti_val_t * val, ex_t * e);
int ti_dict_walk(ti_dict_t * dict, ti_dict_cb cb, void * arg);

#endif  /* TI_DICT_H_ */

