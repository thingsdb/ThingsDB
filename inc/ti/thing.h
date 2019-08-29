/*
 * ti/thing.h
 */
#ifndef TI_THING_H_
#define TI_THING_H_

typedef struct ti_thing_s  ti_thing_t;

#include <qpack.h>
#include <stdint.h>
#include <ti/name.h>
#include <ti/val.h>
#include <ti/raw.h>
#include <ti/prop.h>
#include <ti/watch.h>
#include <ti/stream.h>
#include <util/vec.h>
#include <util/imap.h>

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things);
void ti_thing_destroy(ti_thing_t * thing);
void ti_thing_clear(ti_thing_t * thing);
int ti_thing_props_from_unp(
        ti_thing_t * thing,
        imap_t * things,
        qp_unpacker_t * unp,
        ssize_t sz,
        ex_t * e);
ti_thing_t * ti_thing_new_from_unp(
        qp_unpacker_t * unp,
        imap_t * things,
        ssize_t sz,
        ex_t * e);
ti_val_t * ti_thing_attr_weak_get(ti_thing_t * thing, ti_name_t * name);
ti_prop_t * ti_thing_prop_add(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val);
ti_prop_t * ti_thing_prop_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val);
ti_prop_t * ti_thing_prop_set_e(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e);
_Bool ti_thing_del(ti_thing_t * thing, ti_name_t * name);
int ti_thing_del_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e);
ti_prop_t * ti_thing_weak_get_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e);
_Bool ti_thing_unset(ti_thing_t * thing, ti_name_t * name);
_Bool ti_thing_rename(ti_thing_t * thing, ti_name_t * from, ti_name_t * to);
int ti_thing_gen_id(ti_thing_t * thing);
ti_watch_t * ti_thing_watch(ti_thing_t * thing, ti_stream_t * stream);
_Bool ti_thing_unwatch(ti_thing_t * thing, ti_stream_t * stream);
int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t ** packer, int options);
_Bool ti__thing_has_watchers_(ti_thing_t * thing);
static inline _Bool ti_thing_has_watchers(ti_thing_t * thing);
static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t ** packer);
static inline int ti_thing_id_to_file(ti_thing_t * thing, FILE * f);
static inline int ti_thing_to_map(ti_thing_t * thing);
static inline _Bool ti_thing_is_new(ti_thing_t * thing);
static inline void ti_thing_mark_new(ti_thing_t * thing);
static inline void ti_thing_unmark_new(ti_thing_t * thing);
static inline uint64_t ti_thing_key(ti_thing_t * thing);
static inline ti_val_t * ti_thing_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name);
static inline ti_prop_t * ti_thing_prop_weak_get(
        ti_thing_t * thing,
        ti_name_t * name);

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

static inline _Bool ti_thing_has_watchers(ti_thing_t * thing)
{
    return thing->watchers && ti__thing_has_watchers_(thing);
}

static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t ** packer)
{
    return (qp_add_map(packer) ||
            qp_add_raw(*packer, (const uchar *) TI_KIND_S_THING, 1) ||
            qp_add_int(*packer, thing->id) ||
            qp_close_map(*packer));
}

static inline int ti_thing_id_to_file(ti_thing_t * thing, FILE * f)
{
    return (
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar *) TI_KIND_S_THING, 1) ||
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
    return thing->flags & TI_VFLAG_THING_NEW;
}
static inline void ti_thing_mark_new(ti_thing_t * thing)
{
    thing->flags |= TI_VFLAG_THING_NEW;
}
static inline void ti_thing_unmark_new(ti_thing_t * thing)
{
    thing->flags &= ~TI_VFLAG_THING_NEW;
}
static inline uint64_t ti_thing_key(ti_thing_t * thing)
{
    return (uintptr_t) thing;
}

static inline ti_val_t * ti_thing_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    for (vec_each(thing->props, ti_prop_t, prop))
        if (prop->name == name)
            return prop->val;
    return NULL;
}

static inline ti_prop_t * ti_thing_prop_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    for (vec_each(thing->props, ti_prop_t, prop))
        if (prop->name == name)
            return prop;
    return NULL;
}


#endif /* TI_THING_H_ */
