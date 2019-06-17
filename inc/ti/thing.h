/*
 * ti/thing.h
 */
#ifndef TI_THING_H_
#define TI_THING_H_

enum
{
    TI_THING_FLAG_SWEEP     =1<<0,
    TI_THING_FLAG_NEW       =1<<1,
};

typedef struct ti_thing_s  ti_thing_t;

#include <qpack.h>
#include <stdint.h>
#include <ti/name.h>
#include <ti/val.h>
#include <ti/watch.h>
#include <ti/stream.h>
#include <util/vec.h>
#include <util/imap.h>

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things);
void ti_thing_destroy(ti_thing_t * thing);
void ti_thing_clear(ti_thing_t * thing);
ti_val_t * ti_thing_prop_weak_get(ti_thing_t * thing, ti_name_t * name);
ti_val_t * ti_thing_attr_weak_get(ti_thing_t * thing, ti_name_t * name);
int ti_thing_prop_set(ti_thing_t * thing, ti_name_t * name, ti_val_t * val);
_Bool ti_thing_del(ti_thing_t * thing, ti_name_t * name);
_Bool ti_thing_unset(ti_thing_t * thing, ti_name_t * name);
_Bool ti_thing_rename(ti_thing_t * thing, ti_name_t * from, ti_name_t * to);
int ti_thing_gen_id(ti_thing_t * thing);
ti_watch_t * ti_thing_watch(ti_thing_t * thing, ti_stream_t * stream);
_Bool ti_thing_unwatch(ti_thing_t * thing, ti_stream_t * stream);
int ti_thing_to_packer(
        ti_thing_t * thing,
        qp_packer_t ** packer,
        int flags,
        int fetch);
_Bool ti_thing_has_watchers(ti_thing_t * thing);
static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t ** packer);
static inline int ti_thing_id_to_file(ti_thing_t * thing, FILE * f);
static inline int ti_thing_to_map(ti_thing_t * thing);
static inline _Bool ti_thing_is_new(ti_thing_t * thing);
static inline void ti_thing_mark_new(ti_thing_t * thing);
static inline void ti_thing_unmark_new(ti_thing_t * thing);
static inline uint64_t ti_thing_key(ti_thing_t * thing);

struct ti_thing_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad16;

    uint64_t id;
    imap_t * things;        /* thing is added to this map */
    vec_t * props;          /* vec contains ti_prop_t */
    vec_t * watchers;       /* vec contains ti_watch_t,
                               NULL if no watchers,  */
};

static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t ** packer)
{
    return (qp_add_map(packer) ||
            qp_add_raw(*packer, (const unsigned char *) "#", 1) ||
            qp_add_int(*packer, thing->id) ||
            qp_close_map(*packer));
}

static inline int ti_thing_id_to_file(ti_thing_t * thing, FILE * f)
{
    return (
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar *) "#", 1) ||
            qp_fadd_int(f, thing->id)
    );
}

/* returns IMAP_ERR_EXIST if the thing is already in the map */
static inline int ti_thing_to_map(ti_thing_t * thing)
{
    return imap_add(thing->things, thing->id, thing);
}

static inline _Bool ti_thing_is_new(ti_thing_t * thing)
{
    return thing->flags & TI_THING_FLAG_NEW;
}
static inline void ti_thing_mark_new(ti_thing_t * thing)
{
    thing->flags |= TI_THING_FLAG_NEW;
}
static inline void ti_thing_unmark_new(ti_thing_t * thing)
{
    thing->flags &= ~TI_THING_FLAG_NEW;
}
static inline uint64_t ti_thing_key(ti_thing_t * thing)
{
    return (uintptr_t) thing;
}

#endif /* TI_THING_H_ */
