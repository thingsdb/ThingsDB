/*
 * ti/enum.h
 */
#ifndef TI_ENUM_H_
#define TI_ENUM_H_

#include <ex.h>
#include <inttypes.h>
#include <stdlib.h>
#include <ti/enum.t.h>
#include <ti/member.t.h>
#include <ti/thing.t.h>
#include <ti/val.t.h>
#include <ti/vp.t.h>
#include <ti/vup.t.h>

ti_enum_t * ti_enum_create(
        uint16_t enum_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at);
void ti_enum_destroy(ti_enum_t * enum_);
int ti_enum_prealloc(ti_enum_t * enum_, size_t sz, ex_t * e);
int ti_enum_set_enum_tp(ti_enum_t * enum_, ti_val_t * val, ex_t * e);
int ti_enum_check_val(ti_enum_t * enum_, ti_val_t * val, ex_t * e);
int ti_enum_add_member(ti_enum_t * enum_, ti_member_t * member, ex_t * e);
void ti_enum_del_member(ti_enum_t * enum_, ti_member_t * member);
int ti_enum_add_method(
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_closure_t * closure,
        ex_t * e);
void ti_enum_remove_method(ti_enum_t * enum_, ti_name_t * name);
int ti_enum_init_from_thing(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e);
int ti_enum_init_members_from_vup(ti_enum_t * enum_, ti_vup_t * vup, ex_t * e);
int ti_enum_init_methods_from_vup(ti_enum_t * enum_, ti_vup_t * vup, ex_t * e);
int ti_enum_members_to_client_pk(
        ti_enum_t * enum_,
        ti_vp_t * vp,
        int deep,
        int flags);
int ti_enum_members_to_store_pk(ti_enum_t * enum_, msgpack_packer * pk);
int ti_enum_methods_to_store_pk(ti_enum_t * enum_, msgpack_packer * pk);
ti_member_t * ti_enum_member_by_val(ti_enum_t * enum_, ti_val_t * val);
ti_member_t * ti_enum_member_by_val_e(
        ti_enum_t * enum_,
        ti_val_t * val,
        ex_t * e);
ti_val_t * ti_enum_as_mpval(ti_enum_t * enum_);
int ti_enum_to_pk(ti_enum_t * enum_, ti_vp_t * vp);


#endif  /* TI_MEMBER_H_ */
