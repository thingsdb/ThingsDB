/*
 * thing.h
 */
#ifndef TI_THING_H_
#define TI_THING_H_

#define TI_THING_FLAG_SWEEP 1
#define TI_THING_FLAG_DATA 2

typedef struct ti_thing_s  ti_thing_t;

#include <qpack.h>
#include <stdint.h>
#include <ti/name.h>
#include <ti/api.h>
#include <ti/val.h>
#include <util/vec.h>
#include <util/imap.h>

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things);
void ti_thing_drop(ti_thing_t * thing);

int ti_thing_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_enum tp,
        void * v);
int ti_thing_weak_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_enum tp,
        void * v);
int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t ** packer);
static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t ** packer);

struct ti_thing_s
{
    uint32_t ref;
    uint16_t pad0;
    uint8_t flags;
    uint8_t pad1;
    uint64_t id;
    vec_t * props;
    imap_t * things;       /* thing is added to this map */
};

static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t ** packer)
{
    return (qp_add_map(packer) ||
            qp_add_raw(*packer, (const unsigned char *) "$id", 3) ||
            qp_add_int64(*packer, (int64_t) thing->id) ||
            qp_close_map(*packer));
}

#endif /* TI_THING_H_ */
