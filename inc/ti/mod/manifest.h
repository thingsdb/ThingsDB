/*
 * ti/mod/manifest.h
 */
#ifndef TI_MOD_MANIFEST_H_
#define TI_MOD_MANIFEST_H_

#include <ti/mod/manifest.t.h>

int ti_mod_manifest_read(
        ti_mod_manifest_t * manifest,
        const void * data,
        size_t n);
void ti_mod_manifest_clear(ti_mod_manifest_t * manifest);

#endif  /* TI_MOD_MANIFEST_H_ */
