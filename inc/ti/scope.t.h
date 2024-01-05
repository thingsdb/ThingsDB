/*
 * ti/scope.t.h
 */
#ifndef TI_SCOPE_T_H_
#define TI_SCOPE_T_H_

typedef struct ti_scope_s ti_scope_t;
typedef struct ti_scope_name_s ti_scope_name_t;

typedef enum
{
    TI_SCOPE_THINGSDB,
    TI_SCOPE_NODE,
    TI_SCOPE_COLLECTION,
} ti_scope_enum_t;

#include <stdint.h>

struct ti_scope_name_s
{
    const char * name;
    size_t sz;
};

typedef union
{
    uint32_t node_id;
    ti_scope_name_t collection_name;
} ti_scope_via_t;

struct ti_scope_s
{
    ti_scope_enum_t tp;
    ti_scope_via_t via;
};

#endif /* TI_SCOPE_T_H_ */
