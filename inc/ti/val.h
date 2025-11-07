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

/* name */
extern ti_val_t * val__async_name;
extern ti_val_t * val__data_name;
extern ti_val_t * val__time_name;
extern ti_val_t * val__year_name;
extern ti_val_t * val__month_name;
extern ti_val_t * val__day_name;
extern ti_val_t * val__hour_name;
extern ti_val_t * val__minute_name;
extern ti_val_t * val__second_name;
extern ti_val_t * val__gmt_offset_name;
extern ti_val_t * val__module_name;
extern ti_val_t * val__deep_name;
extern ti_val_t * val__flags_name;
extern ti_val_t * val__load_name;
extern ti_val_t * val__beautify_name;
extern ti_val_t * val__parent_name;
extern ti_val_t * val__parent_type_name;
extern ti_val_t * val__key_name;
extern ti_val_t * val__key_type_name;
extern ti_val_t * val__anonymous_name;

/* string */
extern ti_val_t * val__sany;
extern ti_val_t * val__snil;
extern ti_val_t * val__strue;
extern ti_val_t * val__sfalse;

/* regular expression */
extern ti_val_t * val__re_email;
extern ti_val_t * val__re_url;
extern ti_val_t * val__re_tel;

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
int ti_val_convert_to_bytes(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_int(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_float(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_array(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_set(ti_val_t ** val, ex_t * e);
size_t ti_val_get_len(ti_val_t * val);
int ti_val_gen_ids(ti_val_t * val);
_Bool ti_val_has_ids(ti_val_t * val);
size_t ti_val_alloc_size(ti_val_t * val);
ti_val_t * ti_val_strv(ti_val_t * val);
int ti_val_copy(ti_val_t ** val, ti_thing_t * parent, void * key, uint8_t deep);
int ti_val_dup(ti_val_t ** val, ti_thing_t * parent, void * key, uint8_t deep);

/* `to_str` functions */
int ti_val_nil_to_str(ti_val_t ** val, ex_t * e);
int ti_val_int_to_str(ti_val_t ** val, ex_t * e);
int ti_val_float_to_str(ti_val_t ** val, ex_t * e);
int ti_val_bool_to_str(ti_val_t ** val, ex_t * e);
int ti_val_datetime_to_str(ti_val_t ** val, ex_t * e);
int ti_val_bytes_to_str(ti_val_t ** val, ex_t * e);
int ti_val_regex_to_str(ti_val_t ** val, ex_t * e);
int ti_val_thing_to_str(ti_val_t ** val, ex_t * e);
int ti_val_wrap_to_str(ti_val_t ** val, ex_t * e);
int ti_val_room_to_str(ti_val_t ** val, ex_t * e);
int ti_val_vtask_to_str(ti_val_t ** val, ex_t * e);
int ti_val_error_to_str(ti_val_t ** val, ex_t * e);
int ti_val_member_to_str(ti_val_t ** val, ex_t * e);
int ti_val_closure_to_str(ti_val_t ** val, ex_t * e);

#endif /* TI_VAL_H_ */
