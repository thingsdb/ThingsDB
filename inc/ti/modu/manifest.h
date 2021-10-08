/*
 * ti/modu/manifest.h
 */
#ifndef TI_MODU_MANIFEST_H_
#define TI_MODU_MANIFEST_H_

#include <string.h>
#include <ti/modu/manifest.t.h>
#include <ti/module.t.h>

int ti_modu_manifest_read(
        ti_modu_manifest_t * manifest,
        char * source_err,
        const void * data,
        size_t n);
int ti_modu_manifest_local(ti_modu_manifest_t * manifest, ti_module_t * module);
_Bool ti_modu_manifest_skip_install(
        ti_modu_manifest_t * manifest,
        ti_module_t * module);
int ti_modu_manifest_store(const char * module_path, void * data, size_t n);
void ti_modu_manifest_clear(ti_modu_manifest_t * manifest);

static inline void ti_modu_manifest_init(ti_modu_manifest_t * manifest)
{
    memset(manifest, 0, sizeof(ti_modu_manifest_t));
}

#endif  /* TI_MODU_MANIFEST_H_ */
