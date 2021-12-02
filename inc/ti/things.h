/*
 * ti/things.h
 */
#ifndef TI_THINGS_H_
#define TI_THINGS_H_

#include <stdint.h>
#include <ti/thing.h>
#include <ti/collection.h>
#include <util/imap.h>

ti_thing_t * ti_things_create_thing_o(
        uint64_t id,
        uint16_t spec,
        size_t init_sz,
        ti_collection_t * collection);
ti_thing_t * ti_things_create_thing_t(
        uint64_t id,
        ti_type_t * type,
        ti_collection_t * collection);
ti_thing_t * ti_things_thing_o_from_vup__deprecated(
        ti_vup_t * vup,
        uint64_t thing_id,
        size_t sz,
        ex_t * e);
ti_thing_t * ti_things_thing_o_from_vup(ti_vup_t * vup, ex_t * e);
ti_thing_t * ti_things_thing_t_from_vup(ti_vup_t * vup, ex_t * e);
int ti_things_gc(imap_t * things, ti_thing_t * root);


#endif /* TI_THINGS_H_ */
