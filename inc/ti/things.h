/*
 * things.h
 */
#ifndef TI_ELEMS_H_
#define TI_ELEMS_H_


#include <stdint.h>
#include <ti/thing.h>
#include <util/imap.h>

ti_thing_t * ti_things_create(imap_t * things, uint64_t id);
int ti_things_gc(imap_t * things, ti_thing_t * root);
int ti_things_store(imap_t * things, const char * fn);
int ti_things_store_skeleton(imap_t * things, const char * fn);
int ti_things_store_data(imap_t * things, const char * fn);
int ti_things_restore(imap_t * things, const char * fn);
int ti_things_restore_skeleton(imap_t * things, imap_t * props, const char * fn);
int ti_things_restore_data(imap_t * things, imap_t * props, const char * fn);

struct ti_things_s
{
    ti_prop_t * prop;
    ti_val_t val;
};

#endif /* TI_ELEMS_H_ */
