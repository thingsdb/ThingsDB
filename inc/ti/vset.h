/*
 * ti/vset.h
 */
#ifndef TI_VSET_H_
#define TI_VSET_H_

typedef struct ti_vset_s ti_vset_t;

#include <inttypes.h>
#include <util/imap.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/ex.h>

ti_vset_t * ti_vset_create(void);
void ti_vset_destroy(ti_vset_t * vset);
int ti_vset_to_packer(
        ti_vset_t * vset,
        qp_packer_t ** packer,
        int flags,
        int fetch);
int ti_vset_to_file(ti_vset_t * vset, FILE * f);
int ti_vset_assign(ti_vset_t ** vsetaddr);
int ti_vset_add_val(ti_vset_t * vset, ti_val_t * val, ex_t * e);
static inline int ti_vset_add(ti_vset_t * vset, ti_thing_t * thing);
static inline _Bool ti_vset_has(ti_vset_t * vset, ti_thing_t * thing);
static inline void ti_vset_set_assigned(ti_vset_t * vset);
static inline _Bool ti_vset_is_assigned(ti_vset_t * vset);

struct ti_vset_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad1;
    imap_t * imap;      /* key: thing_key / value: *ti_things_t */
};

/*
 * Returns 0 if the given `thing` is added to the set.
 * (does NOT increment the reference count)
 */
static inline int ti_vset_add(ti_vset_t * vset, ti_thing_t * thing)
{
    return imap_add(vset->imap, ti_thing_key(thing), thing);
}

static inline _Bool ti_vset_has(ti_vset_t * vset, ti_thing_t * thing)
{
    return imap_get(vset->imap, ti_thing_key(thing)) != NULL;
}

static inline void ti_vset_set_assigned(ti_vset_t * vset)
{
    vset->flags &= ~TI_VFLAG_UNASSIGNED;
}

static inline _Bool ti_vset_is_assigned(ti_vset_t * vset)
{
    return ~vset->flags & TI_VFLAG_UNASSIGNED;
}


#endif  /* TI_VSET_H_ */
