/*
 * ti/store/things.h
 */
#ifndef TI_STORE_THINGS_H_
#define TI_STORE_THINGS_H_

int ti_store_things_store(imap_t * things, const char * fn);
int ti_store_things_store_skeleton(imap_t * things, const char * fn);
int ti_store_things_store_data(imap_t * things, _Bool attrs, const char * fn);
int ti_store_things_restore(imap_t * things, const char * fn);
int ti_store_things_restore_skeleton(
        imap_t * things,
        imap_t * names,
        const char * fn);
int ti_store_things_restore_data(
        imap_t * things,
        imap_t * names,
        _Bool attrs,
        const char * fn);
#endif /* TI_STORE_COLLECTIONS_H_ */
