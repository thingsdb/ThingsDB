/*
 * ti/scope.h
 */
#ifndef TI_SCOPE_H_
#define TI_SCOPE_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/pkg.t.h>
#include <ti/scope.t.h>
#include <ti/collection.t.h>
#include <util/vec.h>

int ti_scope_init(ti_scope_t * scope, const char * str, size_t n, ex_t * e);
int ti_scope_init_packed(
        ti_scope_t * scope,
        const unsigned char * data,
        size_t n,
        ex_t * e);
int ti_scope_init_from_up(ti_scope_t * scope, mp_unp_t * up, ex_t * e);

static inline int ti_scope_init_pkg(
        ti_scope_t * scope,
        ti_pkg_t * pkg,
        ex_t * e)
{
    return ti_scope_init_packed(scope, pkg->data, pkg->n, e);
}

static inline _Bool ti_scope_is_collection(ti_scope_t * scope)
{
    return scope->tp == TI_SCOPE_COLLECTION;
}

const char * ti_scope_name_from_id(uint64_t scope_id);
int ti_scope_id(ti_scope_t * scope, uint64_t * scope_id, ex_t * e);
ti_collection_t * ti_scope_get_collection(ti_scope_t * scope, ex_t * e);
void ti_scope_set_tz(uint64_t scope_id, ti_tz_t * tz);
void ti_scope_set_deep(uint64_t scope_id, uint8_t deep);

#endif /* TI_SCOPE_H_ */
