/*
 * ti/name.h
 */
#ifndef TI_NAME_H_
#define TI_NAME_H_

#include <stdint.h>
#include <ti/name.t.h>
#include <util/mpack.h>

ti_name_t * ti_name_create(const char * str, size_t n);
void ti_name_destroy(ti_name_t * name);
_Bool ti_name_is_valid_strn(const char * str, size_t n);

static inline void ti_name_drop(ti_name_t * name)
{
    if (name && !--name->ref)
        ti_name_destroy(name);
}

static inline void ti_name_unsafe_drop(ti_name_t * name)
{
    if (!--name->ref)
        ti_name_destroy(name);
}

/* added for issue #394 */
static inline int ti_name_to_pk(ti_name_t * name, msgpack_packer * pk)
{
    return mp_pack_ext(pk, MPACK_EXT_NAME, name->str, name->n);
}

#endif /* TI_NAME_H_ */
