/*
 * ti/raw.t.h
 */
#ifndef TI_RAW_T_H_
#define TI_RAW_T_H_

typedef struct ti_raw_s ti_raw_t;

#include <inttypes.h>

struct ti_raw_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad0;
    uint32_t n;
    unsigned char data[];
};

#endif  /* TI_RAW_T_H_ */
