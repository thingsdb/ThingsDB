/*
 * ti/things.h
 */
#ifndef TI_THINGS_H_
#define TI_THINGS_H_

#include <stdint.h>
#include <ti/thing.h>
#include <util/imap.h>

ti_thing_t * ti_things_create_thing(imap_t * things, uint64_t id);
int ti_things_gc(imap_t * things, ti_thing_t * root);


#endif /* TI_THINGS_H_ */
