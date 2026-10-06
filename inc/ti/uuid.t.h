/*
 * ti/uuid.t.h
 */
#ifndef TI_UUID_T_H_
#define TI_UUID_T_H_

typedef uint8_t uuid_t[16];
typedef char uuid_raw_t[36];
typedef char uuid_str_t[37];
typedef struct ti_uuid_s  ti_uuid_t;

#include <ti/type.t.h>
#include <ti/val.t.h>

#define VUUID(__x) ((ti_uuid_t *) (__x))->id

struct ti_uuid_s
{
    uint32_t ref;
    uint8_t tp;
    int:24;
    uuid_t id;
};

#endif /* TI_ANO_T_H_ */
