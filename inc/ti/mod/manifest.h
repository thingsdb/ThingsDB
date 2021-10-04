/*
 * ti/mod/manifest.h
 */
#ifndef TI_MOD_MANIFEST_H_
#define TI_MOD_MANIFEST_H_

#include <ti/mod/manifest.t.h>
#include <ti/module.t.h>

int ti_mod_manifest_read(
        ti_mod_manifest_t * manifest,
        char * source_err,
        const void * data,
        size_t n);
int ti_mod_manifest_local(ti_mod_manifest_t * manifest, ti_module_t * module);
_Bool ti_mod_manifest_skip_install(
        ti_mod_manifest_t * manifest,
        ti_module_t * module);
int ti_mod_manifest_store(const char * module_path, void * data, size_t n);
void ti_mod_manifest_clear(ti_mod_manifest_t * manifest);

#endif  /* TI_MOD_MANIFEST_H_ */
