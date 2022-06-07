/*
 * ti/member.inline.h
 */
#ifndef TI_MEMBER_INLINE_H_
#define TI_MEMBER_INLINE_H_

#include <ti/member.h>
#include <ti/val.h>
#include <ti/vp.t.h>
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

#endif  /* TI_MEMBER_INLINE_H_ */
