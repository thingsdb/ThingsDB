/*
 * ti/val.h
 */
#ifndef TI_VAL_H_
#define TI_VAL_H_

#include <ex.h>
#include <stdint.h>
#include <ti/val.t.h>
#include <ti/vup.t.h>
#include <util/vec.h>

int ti_val_init_common(void);
void ti_val_drop_common(void);
void ti_val_destroy(ti_val_t * val);
int ti_val_make_int(ti_val_t ** val, int64_t i);
int ti_val_make_float(ti_val_t ** val, double d);
ti_val_t * ti_val_from_vup(ti_vup_t * vup);
ti_val_t * ti_val_from_vup_e(ti_vup_t * vup, ex_t * e);
ti_val_t * ti_val_empty_str(void);
ti_val_t * ti_val_charset_str(void);
ti_val_t * ti_val_borrow_any_str(void);
ti_val_t * ti_val_borrow_tar_gz_str(void);
ti_val_t * ti_val_borrow_gs_str(void);
ti_val_t * ti_val_empty_bin(void);
ti_val_t * ti_val_wthing_str(void);
ti_val_t * ti_val_utc_str(void);
ti_val_t * ti_val_year_name(void);
ti_val_t * ti_val_month_name(void);
ti_val_t * ti_val_day_name(void);
ti_val_t * ti_val_hour_name(void);
ti_val_t * ti_val_minute_name(void);
ti_val_t * ti_val_second_name(void);
ti_val_t * ti_val_gmt_offset_name(void);
ti_val_t * ti_val_borrow_module_name(void);
ti_val_t * ti_val_borrow_deep_name(void);
vec_t ** ti_val_get_access(ti_val_t * val, ex_t * e, uint64_t * scope_id);
int ti_val_convert_to_str(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_bytes(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_int(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_float(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_array(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_set(ti_val_t ** val, ex_t * e);
_Bool ti_val_as_bool(ti_val_t * val);
size_t ti_val_get_len(ti_val_t * val);
int ti_val_gen_ids(ti_val_t * val);
int ti_val_to_pk(ti_val_t * val, msgpack_packer * pk, int options);
void ti_val_may_change_pack_sz(ti_val_t * val, size_t * sz);
const char * ti_val_str(ti_val_t * val);
ti_val_t * ti_val_strv(ti_val_t * val);

#endif /* TI_VAL_H_ */
