/*
 * ti/vset.h
 */
#ifndef TI_VSET_H_
#define TI_VSET_H_

typedef struct ti_vset_s ti_vset_t;

#define VSET(__x)  ((ti_vset_t *) (__x))->imap

#include <ex.h>
#include <inttypes.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <util/imap.h>
#include <util/mpack.h>

ti_vset_t * ti_vset_create(void);
void ti_vset_destroy(ti_vset_t * vset);
int ti_vset_to_pk(ti_vset_t * vset, msgpack_packer * pk, int options);
int ti_vset_to_list(ti_vset_t ** vsetaddr);
int ti_vset_to_tuple(ti_vset_t ** vsetaddr);
int ti_vset_to_file(ti_vset_t * vset, FILE * f);
int ti_vset_assign(ti_vset_t ** vsetaddr);
int ti_vset_add_val(ti_vset_t * vset, ti_val_t * val, ex_t * e);
_Bool ti__vset_eq(ti_vset_t * va, ti_vset_t * vb);
static inline int ti_vset_add(ti_vset_t * vset, ti_thing_t * thing);
static inline _Bool ti_vset_has(ti_vset_t * vset, ti_thing_t * thing);
static inline ti_thing_t * ti_vset_pop(ti_vset_t * vset, ti_thing_t * thing);
static inline _Bool ti_vset_eq(ti_vset_t * va, ti_vset_t * vb);

struct ti_vset_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t spec;
    imap_t * imap;          /* key: thing_key() / value: *ti_things_t */
    ti_thing_t * parent;    /* without reference,
                               NULL when this is a variable */
    ti_raw_t * key;         /* without reference */
};

/*
 * Returns IMAP_SUCCESS (0) if the given `thing` is added to the set.
 * (does NOT increment the reference count)
 * If the given thing already is in the set, the return value is
 * IMAP_ERR_EXIST (-2) and in case of an memory allocation error the return
 * value is IMAP_ERR_ALLOC (-1).
 */
static inline int ti_vset_add(ti_vset_t * vset, ti_thing_t * thing)
{
    return imap_add(vset->imap, ti_thing_key(thing), thing);
}

static inline _Bool ti_vset_has(ti_vset_t * vset, ti_thing_t * thing)
{
    return imap_get(vset->imap, ti_thing_key(thing)) != NULL;
}

static inline ti_thing_t * ti_vset_pop(ti_vset_t * vset, ti_thing_t * thing)
{
    return imap_pop(vset->imap, ti_thing_key(thing));
}

static inline _Bool ti_vset_eq(ti_vset_t * va, ti_vset_t * vb)
{
    return imap_eq(va->imap, vb->imap);
}

#endif  /* TI_VSET_H_ */
