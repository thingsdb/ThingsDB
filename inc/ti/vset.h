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

struct ti_vset_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad0;
    uint16_t _pad1;
    imap_t * imap;      /* key: thing_id / value: *ti_things_t */
};

#endif  /* TI_VSET_H_ */


/*
 * Returns 0 if the given `thing` is added to the set.
 * (does NOT increment the reference count)
 */
static inline int ti_vset_add(ti_vset_t * vset, ti_thing_t * thing)
{
    return imap_add(vset->imap, thing->id, thing);
}
