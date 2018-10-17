/*
 * props.h
 */
#ifndef TI_PROPS_H_
#define TI_PROPS_H_

#include <stdint.h>
#include <ti/prop.h>
#include <util/imap.h>

void thingsdb_props_init(void);
ti_prop_t * thingsdb_props_get(const char * name);
int thingsdb_props_store(const char * fn);
imap_t * thingsdb_props_restore(const char * fn);
#endif /* TI_PROPS_H_ */
