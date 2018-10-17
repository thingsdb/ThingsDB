/*
 * thing.h
 */
#ifndef TI_ELEM_H_
#define TI_ELEM_H_

#define TI_ELEM_FLAG_SWEEP 1
#define TI_ELEM_FLAG_DATA 2

typedef struct ti_thing_s  ti_thing_t;

#include <qpack.h>
#include <stdint.h>
#include <ti/prop.h>
#include <ti/api.h>
#include <ti/val.h>
#include <util/vec.h>

ti_thing_t * ti_thing_create(uint64_t id);
ti_thing_t * ti_thing_grab(ti_thing_t * thing);
void ti_thing_destroy(ti_thing_t * thing);

int ti_thing_set(ti_thing_t * thing, ti_prop_t * prop, ti_val_e tp, void * v);
int ti_thing_weak_set(
        ti_thing_t * thing,
        ti_prop_t * prop,
        ti_val_e tp,
        void * v);
int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t * packer);
static inline _Bool ti_thing_res_is_id(qp_res_t * res);
static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t * packer);

struct ti_thing_s
{
    uint32_t ref;
    uint8_t flags;
    uint8_t pad0;
    uint16_t db_idx;  /* internal idx in thingsdb->dbs */
    uint64_t id;
    vec_t * items;
};

static inline _Bool ti_thing_res_is_id(qp_res_t * res)
{
    return  res->tp == QP_RES_MAP &&
            res->via.map->n == 1 &&
            !strcmp(res->via.map->keys[0].via.str, TI_API_ID) &&
            res->via.map->values[0].tp == QP_RES_INT64;
}

static inline int ti_thing_id_to_packer(
        ti_thing_t * thing,
        qp_packer_t * packer)
{
    return (qp_add_map(&packer) ||
            qp_add_raw(packer, (const unsigned char *) TI_API_ID, 2) ||
            qp_add_int64(packer, (int64_t) thing->id) ||
            qp_close_map(packer));
}

#endif /* TI_ELEM_H_ */
