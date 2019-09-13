/*
 * ti/scope.h
 */
#ifndef TI_SCOPE_H_
#define TI_SCOPE_H_

#include <inttypes.h>
#include <ti/data.h>
#include <ti/syntax.h>
#include <ti/pkg.h>
#include <ex.h>

#define SCOPES_DOC_ TI_SEE_DOC("#scopes")

typedef struct ti_scope_s ti_scope_t;
typedef struct ti_scope_name_s ti_scope_name_t;

typedef enum
{
    TI_SCOPE_THINGSDB,
    TI_SCOPE_NODE,
    TI_SCOPE_COLLECTION_NAME,
    TI_SCOPE_COLLECTION_ID,
} ti_scope_enum_t;

int ti_scope_init(ti_scope_t * scope, const char * str, size_t n, ex_t * e);
int ti_scope_init_packed(
        ti_scope_t * scope,
        const unsigned char * data,
        size_t n,
        ex_t * e);
static inline int ti_scope_init_pkg(
        ti_scope_t * scope,
        ti_pkg_t * pkg,
        ex_t * e);

struct ti_scope_name_s
{
    const char * name;
    size_t sz;
};

typedef union
{
    uint8_t node_id;
    ti_scope_name_t collection_name;
    uint64_t collection_id;
} ti_scope_via_t;

struct ti_scope_s
{
    ti_scope_enum_t tp;
    ti_scope_via_t via;
};

static inline int ti_scope_init_pkg(
        ti_scope_t * scope,
        ti_pkg_t * pkg,
        ex_t * e)
{
    return ti_scope_init_packed(scope, pkg->data, pkg->n, e);
}


#endif /* TI_SCOPE_H_ */
