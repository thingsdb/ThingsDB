/*
 * ti/member.inline.h
 */
#ifndef TI_MEMBER_INLINE_H_
#define TI_MEMBER_INLINE_H_

#include <ti/member.h>
#include <ti/val.h>
#include <tiinc.h>
#include <util/mpack.h>


static inline const char * ti_member_enum_name(ti_member_t * member)
{
    return member->enum_->name;
}

static inline uint16_t ti_member_enum_id(ti_member_t * member)
{
    return member->enum_->enum_id;
}

static inline ti_raw_t * ti_member_enum_get_rname(ti_member_t * member)
{
    ti_raw_t * rname = member->enum_->rname;
    ti_incref(rname);
    return rname;
}

static inline int ti_member_to_pk(
        ti_member_t * member,
        msgpack_packer * pk,
        int options)
{
    return options >= 0
        ? ti_val_to_pk(member->val, pk, options)
        : -(msgpack_pack_map(pk, 1) ||
            mp_pack_strn(pk, TI_KIND_S_MEMBER, 1) ||
            msgpack_pack_array(pk, 2) ||
            msgpack_pack_uint16(pk, member->enum_->enum_id) ||
            msgpack_pack_uint16(pk, member->idx));
}

#endif  /* TI_MEMBER_INLINE_H_ */
