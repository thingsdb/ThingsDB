/*
 * ti/name.t.h
 */
#ifndef TI_NAME_T_H_
#define TI_NAME_T_H_

/* it is possible to extend the name limit without consequences */
#define TI_NAME_MAX 255

typedef struct ti_name_s ti_name_t;

#include <inttypes.h>

/*
 * name can be cast to raw. the difference is in `char str[]`, but the other
 * fields map to `raw`. therefore the `tp` of name should be equal to `raw`
 */
struct ti_name_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad0;
    uint32_t n;
    char str[];             /* null terminated string */
};

#endif /* TI_NAME_T_H_ */
