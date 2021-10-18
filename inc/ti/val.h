/*
 * ti/val.h
 */
#ifndef TI_VAL_H_
#define TI_VAL_H_

#include <ex.h>
#include <stdint.h>
#include <ti/val.t.h>
#include <ti/vp.t.h>
#include <ti/vup.t.h>
#include <util/vec.h>

extern ti_val_t * val__year_name;
extern ti_val_t * val__month_name;
extern ti_val_t * val__day_name;
extern ti_val_t * val__hour_name;
extern ti_val_t * val__minute_name;
extern ti_val_t * val__second_name;
extern ti_val_t * val__gmt_offset_name;
extern ti_val_t * val__module_name;
extern ti_val_t * val__deep_name;
extern ti_val_t * val__load_name;
extern ti_val_t * val__beautify_name;

int ti_val_init_common(void);
void ti_val_drop_common(void);
int ti_val_make_int(ti_val_t ** val, int64_t i);
int ti_val_make_float(ti_val_t ** val, double d);
ti_val_t * ti_val_from_vup(ti_vup_t * vup);
ti_val_t * ti_val_from_vup_e(ti_vup_t * vup, ex_t * e);
ti_val_t * ti_val_empty_str(void);
ti_val_t * ti_val_default_re(void);
ti_val_t * ti_val_default_closure(void);
ti_val_t * ti_val_charset_str(void);
ti_val_t * ti_val_borrow_any_str(void);
ti_val_t * ti_val_borrow_tar_gz_str(void);
ti_val_t * ti_val_borrow_gs_str(void);
ti_val_t * ti_val_empty_bin(void);
ti_val_t * ti_val_wrapped_thing_str(void);
ti_val_t * ti_val_utc_str(void);
vec_t ** ti_val_get_access(ti_val_t * val, ex_t * e, uint64_t * scope_id);
int ti_val_convert_to_str(ti_val_t ** val, ex_t * e);
void ti_val_ensure_convert_to_str(ti_val_t ** val);
int ti_val_convert_to_bytes(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_int(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_float(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_array(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_set(ti_val_t ** val, ex_t * e);
_Bool ti_val_as_bool(ti_val_t * val);
size_t ti_val_get_len(ti_val_t * val);
int ti_val_gen_ids(ti_val_t * val);
int ti_val_to_pk(ti_val_t * val, ti_vp_t * vp, int options);
size_t ti_val_alloc_size(ti_val_t * val);
const char * ti_val_str(ti_val_t * val);
ti_val_t * ti_val_strv(ti_val_t * val);
int ti_val_copy(ti_val_t ** val, ti_thing_t * parent, void * key, uint8_t deep);
int ti_val_dup(ti_val_t ** val, ti_thing_t * parent, void * key, uint8_t deep);

#endif /* TI_VAL_H_ */
