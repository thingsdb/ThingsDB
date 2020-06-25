/*
 * ti/type.t.h
 */
#ifndef TI_TYPE_T_H_
#define TI_TYPE_T_H_

enum
{
    TI_TYPE_FLAG_LOCK       =1<<0,
};

typedef struct ti_type_s ti_type_t;

#include <inttypes.h>
#include <ti/raw.t.h>
#include <ti/types.t.h>
#include <util/imap.h>
#include <util/smap.h>
#include <util/vec.h>

struct ti_type_s
{
    uint32_t refcount;      /* this counter will be incremented and decremented
                               by other types; the type may only be removed
                               when this counter is equal to zero otherwise
                               other types will break; self references are not
                               included in this counter */
    uint16_t type_id;       /* type id */
    uint8_t flags;          /* type flags */
    uint8_t pad0_;
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    uint64_t modified_at;   /* UNIX time-stamp in seconds */
    char * name;            /* name (null terminated) */
    char * wname;           /* wrapped name (null terminated) */
    ti_raw_t * rname;       /* name as raw type */
    ti_raw_t * rwname;      /* wrapped name as raw type */
    ti_types_t * types;
    vec_t * dependencies;   /* ti_type_t; contains type where this type is
                               depended on. type may be more than one inside
                               this vector but a self dependency is not
                               included, order is not important */
    vec_t * fields;         /* ti_field_t */
    smap_t * methods;       /* method name/ ti_closure_t */
    imap_t * t_mappings;    /* from_type_id / vec_t * with ti_field_t */
};


#endif  /* TI_TYPE_T_H_ */

