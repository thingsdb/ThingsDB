/*
 * elem.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ELEM_H_
#define RQL_ELEM_H_

#define RQL_ELEM_FLAG_SWEEP 1

typedef struct rql_elem_s  rql_elem_t;

#include <qpack.h>
#include <stdint.h>
#include <rql/prop.h>
#include <rql/api.h>
#include <rql/val.h>
#include <util/vec.h>

rql_elem_t * rql_elem_create(uint64_t id);
rql_elem_t * rql_elem_grab(rql_elem_t * elem);
void rql_elem_drop(rql_elem_t * elem);

int rql_elem_set(rql_elem_t * elem, rql_prop_t * prop, rql_val_e tp, void * v);
int rql_elem_weak_set(
        rql_elem_t * elem,
        rql_prop_t * prop,
        rql_val_e tp,
        void * v);
int rql_elem_to_packer(rql_elem_t * elem, qp_packer_t * packer);
static inline _Bool rql_elem_res_is_id(qp_res_t * res);
static inline int rql_elem_id_to_packer(
        rql_elem_t * elem,
        qp_packer_t * packer);

struct rql_elem_s
{
    uint32_t ref;
    uint8_t flags;
    uint8_t pad0;
    uint16_t pad1;
    uint64_t id;
    vec_t * items;
};

static inline _Bool rql_elem_res_is_id(qp_res_t * res)
{
    return  res->tp == QP_RES_MAP &&
            res->via.map->n == 1 &&
            !strcmp(res->via.map->keys[0].via.str, RQL_API_ID) &&
            res->via.map->values[0].tp == QP_RES_INT64;
}

static inline int rql_elem_id_to_packer(
        rql_elem_t * elem,
        qp_packer_t * packer)
{
    return (qp_add_map(&packer) ||
            qp_add_raw(packer, (const unsigned char *) RQL_API_ID, 2) ||
            qp_add_int64(packer, (int64_t) elem->id) ||
            qp_close_map(packer));
}

#endif /* RQL_ELEM_H_ */
