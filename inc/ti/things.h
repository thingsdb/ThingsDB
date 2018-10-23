/*
 * things.h
 */
#ifndef TI_THINGS_H_
#define TI_THINGS_H_


#include <stdint.h>
#include <ti/thing.h>
#include <util/imap.h>

ti_thing_t * ti_things_create_thing(imap_t * things, uint64_t id);
int ti_things_gc(imap_t * things, ti_thing_t * root);
int ti_things_store(imap_t * things, const char * fn);
int ti_things_store_skeleton(imap_t * things, const char * fn);
int ti_things_store_data(imap_t * things, const char * fn);
int ti_things_restore(imap_t * things, const char * fn);
int ti_things_restore_skeleton(imap_t * things, imap_t * names, const char * fn);
int ti_things_restore_data(imap_t * things, imap_t * names, const char * fn);


#endif /* TI_THINGS_H_ */
