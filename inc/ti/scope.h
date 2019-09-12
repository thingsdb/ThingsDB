/*
 * ti/scope.h
 */
#ifndef TI_SCOPE_H_
#define TI_SCOPE_H_

#include <inttypes.h>
#include <ti/data.h>
#include <ex.h>

typedef enum
{
    TI_SCOPE_THINGSDB,
    TI_SCOPE_NODE,
    TI_SCOPE_COLLECTION_NAME,
    TI_SCOPE_COLLECTION_ID,
} ti_scope_enum_t;

typedef union
{
    uint8_t node_id;
    ti_data_t collection_name;
    uint64_t collection_id;
} ti_scope_via_t;

typedef struct ti_scope_s ti_scope_t;

int ti_scope_init(ti_scope_t * scope, unsigned char * data, size_t n, ex_t * e);

struct ti_scope_s
{
    ti_scope_enum_t tp;
    ti_scope_via_t via;
};


#endif /* TI_SCOPE_H_ */
